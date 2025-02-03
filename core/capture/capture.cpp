#include "capture.hpp"

Capture::Capture(int width, int height, int fps, int device)
{
    cap.open(device, cv::CAP_DSHOW);

    if (!cap.isOpened())
    {
        std::cerr << "Error: Unable to initialize webcam." << std::endl;
        return;
    }

    cap.set(cv::CAP_PROP_FRAME_WIDTH, width);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);
    cap.set(cv::CAP_PROP_FPS, fps);
}

Capture::~Capture()
{
    cap.release();
}

//https://stackoverflow.com/questions/30216812/opencv-is-cvmat-like-a-shared-ptr
//Apparently cv::Mat behaves like a shared_ptr so we don't need to worry about ownership etc.
cv::Mat Capture::getFrame()
{
    if (!cap.read(frame))
    {
        std::cerr << "Error: Unable to read frame." << std::endl;

        return cv::Mat();
    }

    return frame;
}