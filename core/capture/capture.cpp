#include "capture.hpp"
#include "../openguard.hpp"

Capture::Capture(int width, int height, int fps, int device)
{
    #if defined(_WIN32)
        cap.open(device, cv::CAP_DSHOW);
    #elif defined(__linux__)
        cap.open(device, cv::CAP_V4L2);
    #else
        cap.open(device);
    #endif

    if (!cap.isOpened())
    {
        Logger::GetInstance().Log("ERROR", "Unable to open capture device.");
        return;
    }

    // Set resolution
    cap.set(cv::CAP_PROP_FRAME_WIDTH, width);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);

    // Attempt to set FPS, some cameras may record at a different rate depending on lighting conditions
    cap.set(cv::CAP_PROP_FPS, fps);

    // Adjust brightness, contrast, and gamma to improve image quality in case of low light
    cap.set(cv::CAP_PROP_BRIGHTNESS, 150);
    cap.set(cv::CAP_PROP_CONTRAST, 50);
    cap.set(cv::CAP_PROP_GAMMA, 200);//todo make this a config option

    this->frame_timer = OpenGuard::Utils::Timer();
    this->fps_timer = OpenGuard::Utils::Timer();
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
        Logger::GetInstance().Log("ERROR", "Unable to read frame.");

        return cv::Mat();
    }

    return frame;
}

cv::VideoCapture Capture::GetCapture()
{
    return cap;
}

int Capture::GetFPS()
{
    return fps;
}

int Capture::GetFrameTime()
{
    return frame_time;
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

    this->frame_time = this->frame_timer.GetDuration() * 1000;
    this->frame_timer.Reset();

    if (this->fps_timer.HasElapsed(1.0))
    {
        this->fps = frame_count;
        this->frame_count = 0;
        this->fps_timer.Reset();
    }
}

