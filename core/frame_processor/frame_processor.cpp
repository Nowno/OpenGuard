#include "frame_processor.hpp"

FrameProcessor::FrameProcessor() {}

FrameProcessor::~FrameProcessor()
{

}

void FrameProcessor::SetMotionDetector(std::unique_ptr<MotionDetector> detector)
{
    this->motion_detector = std::move(detector);
}

void FrameProcessor::ProcessFrame(cv::Mat& frame)
{
    if (!frame.data)
    {
        std::cerr << "Error: Unable to read frame." << std::endl;
        return;
    }

    if (!motion_detector)
    {
        std::cerr << "Error: Motion detector not set." << std::endl;
        return;
    }

    if (motion_detector->Detect(frame))
    {
        cv::putText(frame, "Motion Detected", cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
    }

    this->processed_frame = frame;
}


bool FrameProcessor::RenderFrame()
{
    if (cv::waitKey(1) == 'q')
        return false;

    if (!processed_frame.data)
    {
        std::cerr << "Error: Unable to read frame." << std::endl;
        return false;
    }

    cv::imshow("Frame", processed_frame);

    return true;
}
