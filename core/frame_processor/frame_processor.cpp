#include "frame_processor.hpp"
#include "../../utils/configs/user_config.hpp"
#include "../../utils/logger/logger.hpp"
#include "../hook_manager/hook_manager.hpp"

/**
 * @brief Constructor: Initializes the frame processor.
 * @param cap The capture object.
 */
FrameProcessor::FrameProcessor(Capture& cap) : cap(cap)
{
    this->overlay_renderer = std::make_shared<OverlayRenderer>(cap);
    this->recorder = std::make_unique<Recorder>(cap.GetFrameSize());
}

FrameProcessor::~FrameProcessor()
{
    //todo: maybe add some cleanup here
}

/**
 * @brief Set the motion detector.
 * @param detector The motion detector.
 */
void FrameProcessor::SetMotionDetector(std::unique_ptr<MotionDetector> detector)
{
    //Dependency injection, we are injecting the overlay renderer into the motion detector
    this->motion_detector = std::move(detector);
    this->motion_detector->setOverlayRenderer(overlay_renderer);
}

/**
 * @brief Set the object detector.
 */
void FrameProcessor::SetObjectDetector(std::unique_ptr<ObjectDetector> detector)
{
    this->object_detector = std::move(detector);
    this->object_detector->setOverlayRenderer(overlay_renderer);
}

/**
 * @brief Process the frame.
 */
void FrameProcessor::ProcessFrame(cv::Mat& frame)
{
    /// Ensure frame is valid
    if (!frame.data)
    {
        Logger::GetInstance().Log("ERROR", "Unable to read frame (ProcessFrame).");
        return;
    }

    /// Sanity check
    if (!motion_detector || !object_detector)
    {
        Logger::GetInstance().Log("ERROR", "Detectors not set (ProcessFrame).");
        return;
    }

    /// Run motion detection
    bool motion_detected = motion_detector->Detect(frame);
    static OpenGuard::Utils::StateTracker previous_motion;                        /// [MaybeUnused] The previous motion state
    static ObjectDetector::Object object_detected = ObjectDetector::Object::NONE; /// Previous detected object

    if (motion_detected)
    {
        /// For visibility, add an indicator of motion
        overlay_renderer->Add(OverlayRenderer::OverlayElement(OverlayRenderer::DrawType::TEXT, "Motion", cv::Point(12, 45), cv::Scalar(255, 0, 255), 0.5));

        /// To avoid spamming the logs, only log once per motion event
        if (!previous_motion.GetState())
        {
            previous_motion.SetState(true);                                  /// Update the state
            HookManager::GetInstance().ExecuteHooks("on_motion", {});  /// Trigger the motion hooks
        }

        /// Run object detection
        object_detected = object_detector->Detect(frame);

        if (object_detected != ObjectDetector::Object::NONE)
        {
            /// Again, to avoid spamming the logs, only log if we haven't started recording yet
            if (!recorder->IsRecording())
                HookManager::GetInstance().ExecuteHooks("on_object", {{"object", ObjectDetector::GetObjectString(object_detected)}});
        }
        else
        {
            /// If no object was detected, invalidate the persistent overlay
            /// (because we don't render the objects constantly, we use a persistent overlay to avoid flickering)
            overlay_renderer->InvalidatePersistent();
        }
    }
    else
    {
        previous_motion.SetState(false);
    }

    /// Render the overlay, before recording so that the overlay is also recorded
    overlay_renderer->Render(frame, cap.GetFrameSize());

    /// Add the frame to the recorder
    recorder->AddFrame(frame, motion_detected, object_detected);

    /// Update the processed frame
    this->processed_frame = frame;
}


/**
 * @brief Render the frame.
 */
bool FrameProcessor::RenderFrame()
{
    /// If q is pressed, exit
    if (cv::waitKey(1) == 'q')
        return false;

    /// Ensure processed frame is valid
    if (!processed_frame.data)
    {
        Logger::GetInstance().Log("ERROR", "Unable to render frame (RenderFrame).");
        return false;
    }

    /// Render the processed frame
    cv::imshow("Frame", processed_frame);

    return true;
}

