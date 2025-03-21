#ifndef OPENGUARD_RECORDER_HPP
#define OPENGUARD_RECORDER_HPP
#include <opencv2/opencv.hpp>
#include <string>
#include <deque> // Best choice as it has O(1) for both front and back
#include <atomic>
#include <thread>
#include <mutex>

#include "../detection/object_detector.hpp"

class Recorder
{
    public:
    Recorder(cv::Size frame_size);
    ~Recorder();

    /**
     * @brief Start recording.
     */
    void Start();

    /**
     * @brief Stop recording.
     */
    void Stop();

    /**
     * @brief Add a frame to the recorder.
     * @param frame The frame to add.
     * @param motion_detected Whether motion was detected in the frame.
     * @param object_detected What object was detected in the frame.
     */
    void AddFrame(const cv::Mat& frame, bool motion_detected, ObjectDetector::Object object_detected);

    /**
     * @brief Check if the recorder is currently recording.
     * @return Whether the recorder is currently recording.
     */

    bool IsRecording() const { return recording; }

    private:
    cv::Size frame_size;              /// The size of the frame
    int fps;                          /// The frames per second

    std::string outdir;               /// The output directory
    std::string current_filename;     /// The filename of the current recording

    cv::VideoWriter video_writer;
    std::deque<cv::Mat> frame_buffer; /// Used to store frames before and after recording


    std::atomic<bool> converting{false};       /// Because we detach the thread, we need to ensure it is not running before starting another
    std::thread conversion_thread;             /// The thread used to convert the frames to a video

    bool recording = false;

    int post_record_counter = 0;               /// Used to keep track of how many frames to record after motion stops
    int post_record_buffer_size;               /// The size of the buffer to store frames after motion stops
    int pre_record_buffer_size;                /// The size of the buffer to store frames before motion starts

    ObjectDetector::Object flagged_object = ObjectDetector::Object::NONE; /// The object to flag for conversion

    /**
     * @brief Convert the frames to a video.
     * @param filename The filename of the video.
     * @param object_detected Not necessary, but we pass it as an argument to possible hooks.
     */
    void ConvertToMP4(const std::string& filename, ObjectDetector::Object object_detected);


    std::deque<std::pair<std::string, ObjectDetector::Object>> convert_queue;  /// The queue of videos to convert from avi to mp4
    std::mutex convert_queue_mutex;                                            /// Mutex for the convert queue
};

#endif //OPENGUARD_RECORDER_HPP
