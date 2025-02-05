#include "core/frame_processor/frame_processor.hpp"
#include "core/detection/mog2/mog2.hpp"
#include "core/detection/yolo/yolo.hpp"

int main()
{
    Capture cap(640, 480, 15);

    auto motion_detector = std::make_unique<MOG2Detector>(10000); //see threshold in mog2.hpp
    motion_detector->setDrawBoundingBoxes(true);

    auto object_detector = std::make_unique<YOLODetector>();
    //object_detector->SetAlertObjects({ObjectDetector::Object::PERSON});

    FrameProcessor fp(cap);
    fp.SetMotionDetector(std::move(motion_detector));
    fp.SetObjectDetector(std::move(object_detector));


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
