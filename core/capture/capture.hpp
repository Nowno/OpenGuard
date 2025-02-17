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
    int GetFrameCount();
    int GetFPS();

    void setFPS(int fps);
    void Update();


    private:
    cv::VideoCapture cap;
    cv::Mat frame;

    int frame_count = 0;
    int frame_counter = 0;
    int fps = 0;
};


#endif //OPENGUARD_CAPTURE_HPP
