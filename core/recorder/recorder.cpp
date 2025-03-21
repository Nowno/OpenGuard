#include <iostream>
#include <filesystem>
#include <sstream>
#include <opencv2/videoio.hpp>
#include <unordered_set>
#include <future>

#include "recorder.hpp"
#include "../hook_manager/hook_manager.hpp"

/**
 * @brief Constructor: Initializes the recorder.
 * @param frame_size The size of the frame.
 */
Recorder::Recorder(cv::Size frame_size)
{
    this->fps = ConfigManager::GetInstance().GetConfig<int>("frame_rate");                                  /// Get the frame rate
    this->outdir = ConfigManager::GetInstance().GetConfig<std::string>("output_path");                      /// Get the output path
    this->pre_record_buffer_size = ConfigManager::GetInstance().GetConfig<int>("pre_record_length") * fps;  /// Get the pre-record buffer size by multiplying the pre-record length by the frame rate
    this->post_record_buffer_size = ConfigManager::GetInstance().GetConfig<int>("post_record_length") * fps;/// Same as above, but for post-record
    this->frame_buffer = std::deque<cv::Mat>();                                                             /// Initialize the frame buffer
    this->frame_size = frame_size;                                                                          /// Set the frame size

    /// If the output directory does not exist, create it
    if (!std::filesystem::exists(outdir))
    {
        std::filesystem::create_directories(outdir);
        Logger::GetInstance().Log("INFO", "Output directory created.");
    }
}

/// The program is not really designed to destroy the various objects, so we don't need to do anything here
Recorder::~Recorder()
{
    Stop();
}


/**
 * @brief Add a frame to the recorder.
 */
void Recorder::AddFrame(const cv::Mat& frame, bool motion_detected, ObjectDetector::Object object_detected)
{
    /// This will persist across instances of the recorder, this is a list of objects that are record-worthy
    static std::unordered_set<std::string> record_worthy = ConfigManager::GetInstance().GetConfig<std::unordered_set<std::string>>("yolo_record_worthy");

    /// Add the frame to the buffer
    frame_buffer.push_back(frame.clone());

    /// If the buffer is larger than the pre-record buffer size, remove the oldest frame
    if (frame_buffer.size() > pre_record_buffer_size)
        frame_buffer.pop_front();

    /// Check if the object detected is in the record-worthy list
    bool object_detected_flagged = object_detected != ObjectDetector::Object::NONE && record_worthy.find(ObjectDetector::GetObjectString(object_detected)) != record_worthy.end();

    /// If the object is record-worthy, set the flagged object to the detected object (to be later passed to the converter)
    if (object_detected_flagged)
        flagged_object = object_detected;

    //// If motion is detected and the object is record-worthy, start recording
    if (motion_detected && this->flagged_object != ObjectDetector::Object::NONE)
    {
        if (!recording)
        {
            Logger::GetInstance().Log("INFO", "Motion detected, object: " + ObjectDetector::GetObjectString(object_detected));
            Start();
        }

        /// Reset the post-record counter
        post_record_counter = post_record_buffer_size;
    }
    else if (recording)
    {
        /// Recording but no motion detected, decrement the post-record counter
        post_record_counter--;

        /// If the post-record counter is less than or equal to 0, stop recording
        if (post_record_counter <= 0)
            Stop();
    }

    if (recording)
        video_writer.write(frame);
}

/**
 * @brief Start recording.
 */
void Recorder::Start()
{
    if (recording) /// To not start recording twice
        return;

    Logger::GetInstance().Log("INFO", "Recording started.");

    /// Create a filename for the recording
    std::stringstream filename;
    filename << outdir << "/motion_" << OpenGuard::Utils::GetDateTimeString(true) << ".avi";
    current_filename = filename.str();

    /// Open the video writer, in my testing XVID functioned the best but this is easily alterable
    video_writer.open(current_filename, cv::CAP_FFMPEG, cv::VideoWriter::fourcc('X', 'V', 'I', 'D'), fps, frame_size, true);

    if (!video_writer.isOpened())
    {
        Logger::GetInstance().Log("FATAL", "Could not open video writer.");
        return;
    }

    for (const auto& frame : frame_buffer)    /// Write the frames in the buffer to the video
        video_writer.write(frame);

    frame_buffer.clear();

    recording = true;
}

/**
 * @brief Stop recording.
 */
void Recorder::Stop()
{
    /// If not recording, return as there is nothing to stop
    if (!recording)
        return;

    Logger::GetInstance().Log("INFO", "Recording stopped.");

    video_writer.release();
    recording = false;

    /// Send the video to be converted in a separate thread to not block the main thread
    std::thread(&Recorder::ConvertToMP4, this, current_filename, this->flagged_object).detach();

    /// Reset the flagged object
    this->flagged_object = ObjectDetector::Object::NONE;
}

/**
 * @brief Convert the frames to a video.
 */
void Recorder::ConvertToMP4(const std::string& file, ObjectDetector::Object object_detected)
{
    /// If we are already converting, add the file to the queue and return
    if (converting.exchange(true))
    {
        Logger::GetInstance().Log("INFO", "Added " + file + " to the conversion queue.");

        std::lock_guard<std::mutex> lock(convert_queue_mutex);
        convert_queue.push_back(std::make_pair(file, object_detected));

        return;
    }

    Logger::GetInstance().Log("INFO", "Converting " + file + " to MP4.");

    /// Extract the start time of the recording from the filename
    std::string time_str = file.substr(file.find_last_of('_') + 1, 19); /// Extract the time from the filename
    time_t start_time = OpenGuard::Utils::TimeStringToUnix(time_str, "%Y-%m-%d %H-%M-%S");

    /// Calculate the duration of the recording
    time_t duration = difftime(time(0), start_time);

    /// I suppose this is a way to do metadata injection. I think there's a way to do it with ffmpeg too? But for the sake of simplicity, this is fine.
    std::string object_name = ObjectDetector::GetObjectString(object_detected);
    if (object_name.empty() || object_name == "none")
        object_name = "Unknown";

    /// Extract the output file name without the extension, and append our metadata to it
    std::string out_file = file.substr(0, file.find_last_of('.')) + "_" + std::to_string(duration) + "_" + object_name + ".mp4";

    /// And make the command to invoke FFmpeg to convert the video, disabling output.
    std::stringstream command;
    command << ConfigManager::GetInstance().GetConfig<std::string>("ffmpeg_path") << " -i \"" << file << "\" -vcodec libx264 -crf 23 \"" << out_file << "\" -loglevel error -y >nul 2>&1";

    /// Execute the command and capture the return value
    int ret = std::system(command.str().c_str());

    /// If the return value is 0, the conversion was successful, otherwise log an error
    if (ret == 0)
    {
        Logger::GetInstance().Log("INFO", "Conversion successful.");
        if (!std::filesystem::remove(file))
            Logger::GetInstance().Log("ERROR", "Could not delete " + file);
    }
    else
    {
        Logger::GetInstance().Log("ERROR", "Conversion failed.");
    }


    /// We are no longer converting
    converting = false;

    /// Placeholders for the next file to convert
    std::pair<std::string, ObjectDetector::Object> next_file;

    /// In a separate scope as we are locking the mutex
    {
        std::lock_guard<std::mutex> lock(convert_queue_mutex);
        if (!convert_queue.empty()) /// If there are files in the queue, pop the next one
        {
            next_file = convert_queue.front();
            convert_queue.pop_front();
        }
        else
        {
            converting = false;    /// We are done converting
        }
    }

    /// Execute the on_converted hooks
    HookManager::GetInstance().ExecuteHooks("on_save", {{"file", out_file}, {"object", ObjectDetector::GetObjectString(object_detected)}});

    /// If there is a next file, start converting it
    if (!next_file.first.empty())
    {
       std::thread(&Recorder::ConvertToMP4, this, next_file.first, next_file.second).detach();
    }
}