#include "capture.hpp"

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
cv::Mat Capture::getFrame()
{
    if (!cap.read(frame))
    {
        std::cerr << "Error: Unable to read frame." << std::endl;

        return cv::Mat();
    }

    return frame;
}

cv::VideoCapture Capture::getCapture()
{
    return cap;
}

int Capture::getFrameCount()
{
    return frame_count;
}

int Capture::getFPS()
{
    return fps;
}

Vec2 Capture::getFrameSize()
{
    //Only need to get this once since it won't change
    static int width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    static int height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);

    static Vec2 size(width, height);

    return size;
}

void Capture::Update()
{
    this->frame_count++; // Tracks total frames

    static int frame_counter = 0; // Static local counter for FPS
    frame_counter++;

    static OpenGuard::Utils::Timer timer; // Static timer to track elapsed time

    if (timer.HasElapsed(1.0))
    {
        this->fps = frame_counter; // Store calculated FPS
        frame_counter = 0;         // Reset counter for next second
        timer.Reset();             // Restart the timer
        printf("FPS: %d\n", fps);
    }
}


