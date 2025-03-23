#include "capture.hpp"
#include "../openguard.hpp"

/**
 * @brief Constructor: Initializes the capture object.
 */
Capture::Capture(int width, int height, int fps, int device)
{
    while (!cap.isOpened())
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
            Logger::GetInstance().Log("ERROR", "Unable to open capture device, trying again.");
            return;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    /// Set resolution
    cap.set(cv::CAP_PROP_FRAME_WIDTH, width);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);

    /// Attempt to set FPS, some cameras may record at a different rate depending on lighting conditions
    cap.set(cv::CAP_PROP_FPS, fps);

    /// Adjust brightness, contrast, and gamma to improve image quality in case of low light
    cap.set(cv::CAP_PROP_BRIGHTNESS, 150);
    cap.set(cv::CAP_PROP_CONTRAST, 50);
    cap.set(cv::CAP_PROP_GAMMA, 200);//todo make this a config option

    /// Init timers
    this->frame_timer = OpenGuard::Utils::Timer();
    this->fps_timer = OpenGuard::Utils::Timer();
}

Capture::~Capture()
{
    cap.release();
}

//https://stackoverflow.com/questions/30216812/opencv-is-cvmat-like-a-shared-ptr
//Apparently cv::Mat behaves like a shared_ptr so we don't need to worry about ownership etc.
/**
 * @brief Get the current frame.
 */
cv::Mat Capture::GetFrame()
{
    if (!cap.read(frame))
    {
        Logger::GetInstance().Log("ERROR", "Unable to read frame.");

        return cv::Mat();
    }

    return frame;
}

/**
 * @brief Returns the current capture object.
 */
cv::VideoCapture Capture::GetCapture()
{
    return cap;
}

/**
 * @brief Get the current FPS.
 */
int Capture::GetFPS()
{
    return fps;
}

/**
 * @brief Get the time it took to process the frame in ms.
 */
int Capture::GetFrameTime()
{
    return frame_time;
}

/**
 * @brief Get the size of the frame.
 */
cv::Size Capture::GetFrameSize()
{
    /// Only need to get this once since it won't change
    static int width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    static int height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);

    static cv::Size size(width, height);

    return size;
}

/**
 * @brief Update various states.
 */
void Capture::Update()
{
    this->frame_count++;

    /// Calculate frame time in ms
    this->frame_time = this->frame_timer.GetDuration() * 1000;
    this->frame_timer.Reset();

    /// Not as accurate as it can be but good enough, get the amount of frames there has been in a second
    if (this->fps_timer.HasElapsed(1.0))
    {
        static int frame_rate = ConfigManager::GetInstance().GetConfig<int>("frame_rate");
        static int warn_count = 0;

        this->fps = frame_count;
        this->frame_count = 0;
        this->fps_timer.Reset();

        /// Added this as some cameras may run at a lower frame rate as they make up for low light conditions
        if (this->fps < frame_rate - 7 && warn_count < 5)
        {
            Logger::GetInstance().Log("WARNING", "System running below target FPS.");
            warn_count++;
        }
    }
}

