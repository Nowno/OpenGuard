#include "core/frame_processor/frame_processor.hpp"
#include "core/detection/mog2/mog2.hpp"
#include "core/detection/yolo/yolo.hpp"
#include "core/hook_manager/hook_manager.hpp"

int main()
{
    HookManager test;
    test.RegisterHook("test", HookManager::Hook(HookManager::HookType::NATIVE, [](const std::vector<std::string>& args) -> int
    {
        std::cout << "Hello from hook!" << std::endl;
        return 0;
    }));


    Capture cap(640, 480, 30);

    cv::cvtColor(cap.getFrame(), cap.getFrame(), cv::COLOR_BGR2RGB);

    auto motion_detector = std::make_unique<MOG2Detector>(10000); //see threshold in mog2.hpp
    motion_detector->setDrawBoundingBoxes(true);

    auto object_detector = std::make_unique<YOLODetector>();
    object_detector->setHardwareAcceleration(false);
    //object_detector->SetAlertObjects({ObjectDetector::Object::PERSON});

    FrameProcessor fp(cap);
    fp.SetMotionDetector(std::move(motion_detector));
    fp.SetObjectDetector(std::move(object_detector));

    //print cv build info
    std::cout << cv::getBuildInformation() << std::endl;


    while (true)
    {
        auto frame = cap.getFrame();

        fp.ProcessFrame(frame);

        if (!fp.RenderFrame())
            break;

        cap.Update();
    }


    return 0;
}
