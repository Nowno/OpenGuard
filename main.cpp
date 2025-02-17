#include "core/frame_processor/frame_processor.hpp"
#include "core/detection/mog2/mog2.hpp"
#include "core/detection/yolo/yolo.hpp"
#include "core/hook_manager/hook_manager.hpp"
#include "utils/logger/logger.hpp"

int main()
{
    //Set up capture
    Capture cap(640, 480, 30);

    //Set up detectors
    auto motion_detector = std::make_unique<MOG2Detector>(10000); //todo: maybe take this from user config
    motion_detector->setDrawBoundingBoxes(false);

    auto object_detector = std::make_unique<YOLODetector>();
    object_detector->setHardwareAcceleration(false);
    object_detector->setDrawBoundingBoxes(false);

    //Create frame processor and assign motion and object detectors
    FrameProcessor fp(cap);
    fp.SetMotionDetector(std::move(motion_detector));
    fp.SetObjectDetector(std::move(object_detector));

    //Register hooks
    // 1 - Bulk register external hooks in their designated directories
    HookManager& hook_manager = HookManager::GetInstance();
    hook_manager.RegisterHooks("hooks/on_start");
    hook_manager.RegisterHooks("hooks/on_motion");
    hook_manager.RegisterHooks("hooks/on_object");
    hook_manager.RegisterHooks("hooks/on_save");

    // 2 - Register native hooks manually
    hook_manager.RegisterHook("on_hook", HookManager::Hook(HookManager::HookType::NATIVE, [](const std::unordered_map<std::string, std::string>& args) -> int {
        Logger::GetInstance().Log("INFO", args.at("event") + " events called.");
        return 0;
    }, false));

    hook_manager.RegisterHook("on_object", HookManager::Hook(HookManager::HookType::NATIVE, [](const std::unordered_map<std::string, std::string>& args) -> int {
        Logger::GetInstance().Log("INFO", "detected object: " + args.at("object"));
        return 0;
    },false));

    // 3 - Execute on_start hooks
    hook_manager.ExecuteHooks("on_start", {{"time", std::to_string(time(0))}});

    //Log application start
    Logger::GetInstance().Log("INFO", "Application started.");

    while (true)
    {
        auto frame = cap.GetFrame();

        fp.ProcessFrame(frame);

        if (!fp.RenderFrame())
            break;

        cap.Update();
    }

    //todo: setup script py
    return 0;
}
