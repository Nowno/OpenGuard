#include "frame_processor.hpp"
#include "../../util/configs/user_config.hpp"
//todo: smart ptr
FrameProcessor::FrameProcessor(Capture& cap) : cap(cap)
{
    this->overlay_renderer = std::make_shared<OverlayRenderer>();
    this->recorder = std::make_unique<Recorder>(cap.GetFrameSize(), 15);
}

FrameProcessor::~FrameProcessor()
{

}

//todo: more error handling here
void FrameProcessor::SetMotionDetector(std::unique_ptr<MotionDetector> detector)
{
    this->motion_detector = std::move(detector);
    //Dependency injection, we are injecting the overlay renderer into the motion detector
    this->motion_detector->setOverlayRenderer(overlay_renderer);
}

void FrameProcessor::SetObjectDetector(std::unique_ptr<ObjectDetector> detector)
{
    this->object_detector = std::move(detector);
    //Same as above
    this->object_detector->setOverlayRenderer(overlay_renderer);
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

    bool motion_detected = motion_detector->Detect(frame);

    if (motion_detected)
    {
        overlay_renderer->AddElement(OverlayRenderer::OverlayElement(OverlayRenderer::DrawType::TEXT, "Motion Detected", cv::Point(10, 30), cv::Scalar(0, 0, 255), 1));

        static int frame_count = 0;
        frame_count++;

        ObjectDetector::Object object_detected = ObjectDetector::Object::NONE;

        if (frame_count % 3 == 0) //todo: make it configurable
        {
            object_detected = object_detector->Detect(frame);
            frame_count = 0;
        }

        //overlay_renderer->AddElement(OverlayRenderer::OverlayElement(OverlayRenderer::DrawType::TEXT, object_string, cv::Point(10, 60), cv::Scalar(0, 255, 0), 1));
    }
    recorder->AddFrame(frame, motion_detected);




    //Todo: In report talk about how I went to optimise the text rendering but then turns out it wasnt the issue but still worth it
    overlay_renderer->AddElement(OverlayRenderer::OverlayElement(OverlayRenderer::DrawType::TEXT, "FPS: " + std::to_string(cap.GetFPS()), cv::Point(10, 50), cv::Scalar(0, 0, 255), 1));
    overlay_renderer->Render(frame, cap.GetFrameSize());

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

