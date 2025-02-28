#include "core/frame_processor/frame_processor.hpp"
#include "core/detection/mog2/mog2.hpp"
#include "core/detection/yolo/yolo.hpp"
#include "core/hook_manager/hook_manager.hpp"
#include "utils/logger/logger.hpp"

int main()
{
    /// Log application start
    Logger::GetInstance().Log("INFO", "Application started.");

    /// Set up capture
    Capture cap(ConfigManager::GetInstance().GetConfig<int>("frame_width"),
                ConfigManager::GetInstance().GetConfig<int>("frame_height"),
                ConfigManager::GetInstance().GetConfig<int>("frame_rate"));

    /// Set up detectors
    auto motion_detector = std::make_unique<MOG2Detector>();
    motion_detector->setDrawBoundingBoxes(false);

    auto object_detector = std::make_unique<YOLODetector>();
    object_detector->setDrawBoundingBoxes(true);

    /// Create frame processor and assign motion and object detectors
    FrameProcessor fp(cap);
    fp.SetMotionDetector(std::move(motion_detector));
    fp.SetObjectDetector(std::move(object_detector));

    /// Register hooks
    /// 1 - Bulk register external hooks in their designated directories
    HookManager& hook_manager = HookManager::GetInstance();
    hook_manager.RegisterHooks("hooks/on_start");
    hook_manager.RegisterHooks("hooks/on_motion");
    hook_manager.RegisterHooks("hooks/on_object");
    hook_manager.RegisterHooks("hooks/on_save");
    hook_manager.RegisterHooks("hooks/on_fatal");

    /// 2 - Register native hooks manually
    hook_manager.RegisterHook("on_hook", HookManager::Hook(HookManager::HookType::NATIVE, [](const std::unordered_map<std::string, std::string>& args) -> int {
        Logger::GetInstance().Log("INFO", args.at("event") + " events called.");
        return 0;
    }, false));

    /// 3 - Execute on_start hooks
    hook_manager.ExecuteHooks("on_start", {});

    while (true)
    {
        auto frame = cap.GetFrame();

        auto motion_hook_output = hook_manager.GetHookOutput("on_motion", "pause");

        /// If the motion hook output is not empty and the cooldown has not expired, skip the frame
        if (!motion_hook_output.empty() && std::stoi(motion_hook_output) - time(0) > 0)
            continue;

        fp.ProcessFrame(frame);

        if (!fp.RenderFrame())
            break;

        cap.Update();
    }

    return 0;
}
