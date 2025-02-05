#include "frame_processor.hpp"

FrameProcessor::FrameProcessor(Capture& cap) : cap(cap)
{

}

FrameProcessor::~FrameProcessor()
{

}

void FrameProcessor::SetMotionDetector(std::unique_ptr<MotionDetector> detector)
{
    this->motion_detector = std::move(detector);
}

void FrameProcessor::SetObjectDetector(std::unique_ptr<ObjectDetector> detector)
{
    this->object_detector = std::move(detector);
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
/*
    if (motion_detector->Detect(frame))
    {
        cv::putText(frame, "Motion Detected", cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);

        if (object_detector->Detect(frame) == ObjectDetector::Object::PERSON)
        {
            cv::putText(frame, "Person Detected", cv::Point(10, 60), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2);
        }

    }*/


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

    FrameProcessor::RenderOverlay([&](cv::Mat& overlay)
    {
        cv::putText(overlay, "FPS: " + std::to_string(cap.getFPS()), cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
    });

    cv::imshow("Frame", processed_frame);

    return true;
}

void FrameProcessor::RenderOverlay(std::function<void(cv::Mat&)> overlay)
{
    static cv::Mat draw_overlay = cv::Mat::zeros(cap.getFrameSize().y, cap.getFrameSize().x, CV_8UC3);
    overlay(draw_overlay);
    cv::addWeighted(processed_frame, 1.0, draw_overlay, 0.7, 0, processed_frame);
}
