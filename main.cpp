#include "core/capture/capture.hpp"
#include "core/frame_processor/frame_processor.hpp"
#include "core/detection/mog2/mog2.hpp"

int main()
{
    Capture cap(640, 480, 30);

    auto motionDetector = std::make_unique<MOG2Detector>(10000); //see threshold in mog2.hpp
    motionDetector->SetDrawBoundingBoxes(true);

    FrameProcessor fp;
    fp.SetMotionDetector(std::move(motionDetector));


    while (true)
    {
        auto frame = cap.getFrame();

        fp.ProcessFrame(frame);

        if (!fp.RenderFrame())
            break;
    }

    return 0;
}
