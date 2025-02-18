#include "frame_processor.hpp"
#include "../../utils/configs/user_config.hpp"
#include "../../utils/logger/logger.hpp"
#include "../hook_manager/hook_manager.hpp"
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
        Logger::GetInstance().Log("ERROR", "Unable to read frame (ProcessFrame).");
        return;
    }

    if (!motion_detector || !object_detector)
    {
        Logger::GetInstance().Log("ERROR", "Detectors not set (ProcessFrame).");
        return;
    }

    bool motion_detected = motion_detector->Detect(frame);

    static OpenGuard::Utils::Timer timer;
    static ObjectDetector::Object object_detected = ObjectDetector::Object::NONE;
    //todo: record on motion/record on object options

    if (motion_detected)
    {
        overlay_renderer->Add(OverlayRenderer::OverlayElement(OverlayRenderer::DrawType::TEXT, "Motion Detected", cv::Point(10, 30), cv::Scalar(0, 0, 255), 1));
        if (!recorder->IsRecording())
        {
            //HookManager::GetInstance().ExecuteHooks("on_motion", {});
        }


       /* if (object_detector->Detect(frame) != ObjectDetector::Object::NONE)
        {
            if (!object_state.GetPreviousState())
            {
                object_state.SetPreviousState(true);

                HookManager::GetInstance().ExecuteHooks("on_object", {{"object", ObjectDetector::GetObjectString(object_detected)}});
            }

            std::string object_string = object_detector->GetObjectString(object_detected);

            Logger::GetInstance().Log("INFO", "Object detected: " + object_string);

            overlay_renderer->Add(OverlayRenderer::OverlayElement(OverlayRenderer::DrawType::TEXT, object_string, cv::Point(10, 60), cv::Scalar(0, 255, 0), 1));
        }*/
    }



    overlay_renderer->Add(OverlayRenderer::OverlayElement(OverlayRenderer::DrawType::TEXT, "FPS: " +std::to_string(cap.GetFPS()), cv::Point(10, 50), cv::Scalar(0, 0, 255), 1));
    overlay_renderer->Render(frame, cap.GetFrameSize());

    //recorder->AddFrame(frame, motion_detected, object_detected);


    this->processed_frame = frame;
}


bool FrameProcessor::RenderFrame()
{
    if (cv::waitKey(1) == 'q')
        return false;

    if (!processed_frame.data)
    {
        Logger::GetInstance().Log("ERROR", "Unable to render frame (RenderFrame).");
        return false;
    }

    cv::imshow("Frame", processed_frame);

    return true;
}

