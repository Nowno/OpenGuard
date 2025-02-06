#include "frame_processor.hpp"

//todo: smart ptr
FrameProcessor::FrameProcessor(Capture& cap) : cap(cap)
{
    this->overlay_renderer = std::make_shared<OverlayRenderer>();
}

FrameProcessor::~FrameProcessor()
{

}

//todo: more error handling here
void FrameProcessor::SetMotionDetector(std::unique_ptr<MotionDetector> detector)
{
    this->motion_detector = std::move(detector);
    //Dependency injection, we are injecting the overlay renderer into the motion detector
    this->motion_detector->SetOverlayRenderer(overlay_renderer);
}

void FrameProcessor::SetObjectDetector(std::unique_ptr<ObjectDetector> detector)
{
    this->object_detector = std::move(detector);
    //Same as above
    this->object_detector->SetOverlayRenderer(overlay_renderer);
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
        overlay_renderer->AddElement(OverlayRenderer::OverlayElement(OverlayRenderer::DrawType::TEXT, "Motion Detected", cv::Point(10, 30), cv::Scalar(0, 0, 255), 1));

        if (object_detector->Detect(frame) == ObjectDetector::Object::PERSON)
        {
            overlay_renderer->AddElement(OverlayRenderer::OverlayElement(OverlayRenderer::DrawType::TEXT, "Person Detected", cv::Point(10, 60), cv::Scalar(0, 255, 0), 1));
        }
    }

    //Todo: In report talk about how I went to optimise the text rendering but then turns out it wasnt the issue but still worth it
    overlay_renderer->AddElement(OverlayRenderer::OverlayElement(OverlayRenderer::DrawType::TEXT, "FPS: " + std::to_string(cap.getFPS()), cv::Point(10, 50), cv::Scalar(0, 0, 255), 1));
    overlay_renderer->Render(frame, cap.getFrameSize());

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

