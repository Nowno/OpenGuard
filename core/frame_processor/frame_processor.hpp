#ifndef OPENGUARD_FRAME_PROCESSOR_HPP
#define OPENGUARD_FRAME_PROCESSOR_HPP
#include <opencv2/opencv.hpp>
#include "../capture/capture.hpp"
#include "../detection/motion_detector.hpp"
#include "../detection/object_detector.hpp"

class FrameProcessor
{
    public:
    FrameProcessor(Capture& cap); // Passed by reference as we will alter the frame
    ~FrameProcessor();

    void ProcessFrame(cv::Mat& frame);
    bool RenderFrame();

    // For future flexibility, allow the motion detector to be altered at runtime
    void SetMotionDetector(std::unique_ptr<MotionDetector> detector);
    void SetObjectDetector(std::unique_ptr<ObjectDetector> detector);

    private:
    std::unique_ptr<MotionDetector> motion_detector;
    std::unique_ptr<ObjectDetector> object_detector;

    std::shared_ptr<OverlayRenderer> overlay_renderer;

    cv::Mat processed_frame;
    Capture& cap;

    bool draw_fps = false;
    bool draw_overlay = false;
};


#endif //OPENGUARD_FRAME_PROCESSOR_HPP
