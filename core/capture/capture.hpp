#ifndef OPENGUARD_CAPTURE_HPP
#define OPENGUARD_CAPTURE_HPP
//todo: optimise imports
#include <opencv2/opencv.hpp>
#include "../../utils/utils.hpp"

class Capture
{
    public:
    Capture(int width, int height, int fps, int device = 0);
    ~Capture();

    cv::Mat GetFrame();
    cv::VideoCapture GetCapture();

    cv::Size GetFrameSize();

    int GetFPS();
    int GetFrameTime();

    void Update();


    private:
    cv::VideoCapture cap;
    cv::Mat frame;

    OpenGuard::Utils::Timer fps_timer;
    OpenGuard::Utils::Timer frame_timer;

    int frame_count = 0;
    int frame_time = 0;
    int fps = 0;
};


#endif //OPENGUARD_CAPTURE_HPP
