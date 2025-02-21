#include "frame_processor.hpp"
#include "../../utils/configs/user_config.hpp"
#include "../../utils/logger/logger.hpp"
#include "../hook_manager/hook_manager.hpp"
//todo: smart ptr
FrameProcessor::FrameProcessor(Capture& cap) : cap(cap)
{
    this->overlay_renderer = std::make_shared<OverlayRenderer>(cap);
    this->recorder = std::make_unique<Recorder>(cap.GetFrameSize(), 15);
}

FrameProcessor::~FrameProcessor()
{

}

void FrameProcessor::SetMotionDetector(std::unique_ptr<MotionDetector> detector)
{
    //Dependency injection, we are injecting the overlay renderer into the motion detector
    this->motion_detector = std::move(detector);
    this->motion_detector->setOverlayRenderer(overlay_renderer);
}

void FrameProcessor::SetObjectDetector(std::unique_ptr<ObjectDetector> detector)
{
    this->object_detector = std::move(detector);
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
    static OpenGuard::Utils::StateTracker previous_motion;

    static ObjectDetector::Object object_detected = ObjectDetector::Object::NONE;

    if (motion_detected)
    {
        overlay_renderer->Add(OverlayRenderer::OverlayElement(OverlayRenderer::DrawType::TEXT, "Motion", cv::Point(12, 45), cv::Scalar(255, 0, 255), 0.5));

        if (!previous_motion.GetState())
        {
            previous_motion.SetState(true);

            Logger::GetInstance().Log("INFO", "Motion triggered.", false);
            HookManager::GetInstance().ExecuteHooks("on_motion", {});
        }

        object_detected = object_detector->Detect(frame);

        if (object_detected != ObjectDetector::Object::NONE)
        {
            if (!recorder->IsRecording())
            {
                HookManager::GetInstance().ExecuteHooks("on_object", {{"object", ObjectDetector::GetObjectString(object_detected)}});
            }

            std::string object_string = object_detector->GetObjectString(object_detected);
        }
        else
        {
            overlay_renderer->InvalidatePersistent();
        }
    }
    else
    {
        previous_motion.SetState(false);
    }


    overlay_renderer->Render(frame, cap.GetFrameSize());

    recorder->AddFrame(frame, motion_detected, object_detected);

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

