#include "recorder.hpp"
#include "../../utils/configs/user_config.hpp"
#include "../../utils/utils.hpp"
#include <iostream>
#include <filesystem>
#include <sstream>
#include <opencv2/videoio.hpp>
#include <unordered_set>
//todo: comments
Recorder::Recorder(cv::Size frame_size, int fps) : frame_size(frame_size), fps(fps), recording(false)
{
    outdir = ConfigManager::GetInstance().GetConfig<std::string>("output_path");
    pre_record_buffer_size = 60;
    post_record_buffer_size = 60;
    frame_buffer = std::deque<cv::Mat>();

    std::filesystem::create_directories(outdir);
    if (!std::filesystem::exists(outdir))
    {
        std::cerr << "Failed to create output directory: " << outdir << std::endl;
    }
}

Recorder::~Recorder()
{
    Stop();
}

void Recorder::AddFrame(const cv::Mat& frame, bool motion_detected, ObjectDetector::Object object_detected)
{
    static int post_record_counter = 0;
    static std::unordered_set<std::string> record_worthy = ConfigManager::GetInstance().GetConfig<std::unordered_set<std::string>>("record_worthy");

    frame_buffer.push_back(frame.clone());

    if (frame_buffer.size() > pre_record_buffer_size)
        frame_buffer.pop_front();

    bool object_detected_flagged = record_worthy.find(ObjectDetector::GetObjectString(object_detected)) != record_worthy.end();

    std::cout << "Object detected: " << object_detected_flagged << std::endl;

    if (object_detected_flagged)
        flagged_object = object_detected;

    if (motion_detected && this->flagged_object != ObjectDetector::Object::NONE)
    {
        std::cout << "Motion frfr: " << ObjectDetector::GetObjectString(this->flagged_object) << std::endl;
        if (!recording)
            Start();

        post_record_counter = post_record_buffer_size;
    }
    else if (recording)
    {
        post_record_counter--;

        if (post_record_counter <= 0)
            Stop();
    }

    if (recording)
        video_writer.write(frame);
}

void Recorder::Start()
{
    if (recording)
        return;

    Logger::GetInstance().Log("INFO", "Recording started.");

    std::stringstream filename;
    filename << outdir << "/motion_" << OpenGuard::Utils::DateTimeString() << ".avi";
    current_filename = filename.str();

    video_writer.open(current_filename, cv::CAP_FFMPEG, cv::VideoWriter::fourcc('X', 'V', 'I', 'D'), fps, frame_size, true);
    if (!video_writer.isOpened())
    {
        //todo: proper error logging
        return;
    }

    for (const auto& frame : frame_buffer)
        video_writer.write(frame);

    frame_buffer.clear();

    recording = true;
}

void Recorder::Stop()
{
    if (!recording) return;

    std::cout << "Recording stopped: " << current_filename << std::endl;
    video_writer.release();
    recording = false;

    std::thread(&Recorder::ConvertToMP4, this, current_filename, this->flagged_object).detach();

    this->flagged_object = ObjectDetector::Object::NONE;
}


void Recorder::ConvertToMP4(const std::string& file, ObjectDetector::Object object_detected)
{
    if (converting.exchange(true))
    {
        //todo: proper error logging
        std::lock_guard<std::mutex> lock(convert_queue_mutex);
        convert_queue.push_back(std::make_pair(file, object_detected));
        return;
    }

    std::string out_file = file.substr(0, file.find_last_of('.')) + ".mp4";
    std::stringstream command;
    command << ConfigManager::GetInstance().GetConfig<std::string>("ffmpeg_path") << " -i \"" << file << "\" -vcodec libx264 -crf 23 \"" << out_file << "\" -loglevel error -y >nul 2>&1";
    int ret = std::system(command.str().c_str());  // Executes FFmpeg

    if (ret == 0)
    {
        std::filesystem::remove(file);
    }
    else
    {
    }

    converting = false;

    std::pair<std::string, ObjectDetector::Object> next_file;
    {
        std::lock_guard<std::mutex> lock(convert_queue_mutex);
        if (!convert_queue.empty())
        {
            next_file = convert_queue.front();
            convert_queue.pop_front();
        }
        else
        {
            converting = false;
        }
    }

    //todo: hook here on_converted

    if (!next_file.first.empty())
    {
        std::thread(&Recorder::ConvertToMP4, this, next_file.first, next_file.second).detach();
    }
}