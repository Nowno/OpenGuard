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
    Recorder(cv::Size frame_size, int fps);
    ~Recorder();

    void Start();
    void Stop();
    void AddFrame(const cv::Mat& frame, bool motion_detected, ObjectDetector::Object object_detected);

    bool IsRecording() const { return recording; }

    private:
    cv::Size frame_size;
    int fps;

    std::string outdir;
    std::string current_filename;

    cv::VideoWriter video_writer;
    std::deque<cv::Mat> frame_buffer;

    std::atomic<bool> converting{false};
    std::thread conversion_thread;

    bool recording;

    int post_record_counter;
    int post_record_buffer_size;
    int pre_record_buffer_size;

    ObjectDetector::Object flagged_object;

    void ConvertToMP4(const std::string& filename, ObjectDetector::Object object_detected);

    std::deque<std::pair<std::string, ObjectDetector::Object>> convert_queue;
    std::mutex convert_queue_mutex;
};

#endif //OPENGUARD_RECORDER_HPP
