#include "capture.hpp"
#include "../openguard.hpp"

Capture::Capture(int width, int height, int fps, int device)
{
    //todo: api preference
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
cv::Mat Capture::GetFrame()
{
    if (!cap.read(frame))
    {
        std::cerr << "Error: Unable to read frame." << std::endl;

        return cv::Mat();
    }

    return frame;
}

cv::VideoCapture Capture::GetCapture()
{
    return cap;
}

int Capture::GetFrameCount()
{
    return frame_count;
}

int Capture::GetFPS()
{
    return fps;
}

cv::Size Capture::GetFrameSize()
{
    //Only need to get this once since it won't change
    static int width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    static int height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);

    static cv::Size size(width, height);

    return size;
}

void Capture::Update()
{
    this->frame_count++;

    static OpenGuard::Utils::Timer timer; // Static timer to track elapsed time

    if (timer.HasElapsed(1.0))  // If one second has passed
    {
        this->fps = frame_count;  // Save the frame count as FPS
        frame_count = 0;  // Reset frame counter for the next second
        timer.Reset();
        printf("FPS: %d\n", this->fps);
    }
}

