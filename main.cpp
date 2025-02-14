#include "core/frame_processor/frame_processor.hpp"
#include "core/detection/mog2/mog2.hpp"
#include "core/detection/yolo/yolo.hpp"
#include "core/hook_manager/hook_manager.hpp"
#include "util/logger/logger.hpp"

int main()
{
    Capture cap(640, 480, 30);

    //Set up detectors
    auto motion_detector = std::make_unique<MOG2Detector>(10000); //todo: maybe take this from user config
    motion_detector->setDrawBoundingBoxes(true);

    auto object_detector = std::make_unique<YOLODetector>();
    object_detector->setHardwareAcceleration(false);

    //Create frame processor and assign motion and object detectors
    FrameProcessor fp(cap);
    fp.SetMotionDetector(std::move(motion_detector));
    fp.SetObjectDetector(std::move(object_detector));

    //Register hooks
    // 1 - Bulk register hooks in their designated directories
    HookManager& hook_manager = HookManager::GetInstance();
    hook_manager.RegisterHooks("hooks/on_start");
    hook_manager.RegisterHooks("hooks/on_motion");
    hook_manager.RegisterHooks("hooks/on_object");
    hook_manager.RegisterHooks("hooks/on_record_stop");
    hook_manager.RegisterHooks("hooks/on_video_save");

    // 2 - Register individual hooks
    hook_manager.RegisterHook("on_hook", HookManager::Hook(HookManager::HookType::NATIVE, [](const std::unordered_map<std::string, std::string>& args) -> int {
        Logger::GetInstance().Log("INFO", args.at("hook_name") + " hook called.");
        return 0;
    }, false));

    while (true)
    {
        auto frame = cap.GetFrame();

        fp.ProcessFrame(frame);

        if (!fp.RenderFrame())
            break;

        cap.Update();
    }


    return 0;
}
