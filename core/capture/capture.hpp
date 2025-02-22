#ifndef OPENGUARD_CAPTURE_HPP
#define OPENGUARD_CAPTURE_HPP
//todo: optimise imports
#include <opencv2/opencv.hpp>
#include "../../utils/utils.hpp"

class Capture
{
    public:
    /**
     * @brief Constructor: Initializes the capture object.
     * @param width The width of the frame.
     * @param height The height of the frame.
     * @param fps The FPS of the capture.
     * @param device The device to capture from.
     */
    Capture(int width, int height, int fps, int device = 0);
    ~Capture();

    /**
     * @brief Get the current frame.
     * @return cv::Mat The current frame.
     */
    cv::Mat GetFrame();

    /**
     * @brief Returns the current capture object.
     * @return cv::VideoCapture
     */
    cv::VideoCapture GetCapture();

    /**
     * @brief Get the size of the frame.
     * @return the size of the frame in cv::Size
     */
    cv::Size GetFrameSize();

    /**
     * @brief Get the current FPS.
     * @return int
     */
    int GetFPS();

    /**
     * @brief Get the time it took to process the frame in ms.
     * @return int
     */
    int GetFrameTime();

    /**
     * @brief Update various states.
     */
    void Update();


    private:
    cv::VideoCapture cap; /// The capture object.
    cv::Mat frame;        /// The current frame.


    OpenGuard::Utils::Timer fps_timer;   /// Timer for FPS calculation.
    OpenGuard::Utils::Timer frame_timer; /// Timer for frame processing.

    /// Internal state variables.
    int frame_count = 0;
    int frame_time = 0;
    int fps = 0;
};


#endif //OPENGUARD_CAPTURE_HPP
