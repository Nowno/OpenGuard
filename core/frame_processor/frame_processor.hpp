#ifndef OPENGUARD_FRAME_PROCESSOR_HPP
#define OPENGUARD_FRAME_PROCESSOR_HPP
#include <opencv2/opencv.hpp>
#include "../detection/motion_detector.hpp"
class FrameProcessor
{
    private:
    std::unique_ptr<MotionDetector> motion_detector;
    cv::Mat processed_frame;

    public:
    FrameProcessor(); // Passed by reference as we will alter the frame
    ~FrameProcessor();

    void ProcessFrame(cv::Mat& frame);
    bool RenderFrame();

    // For future flexibility, allow the motion detector to be altered at runtime
    void SetMotionDetector(std::unique_ptr<MotionDetector> detector);
};


#endif //OPENGUARD_FRAME_PROCESSOR_HPP
