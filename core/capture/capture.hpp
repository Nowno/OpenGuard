#ifndef OPENGUARD_CAPTURE_HPP
#define OPENGUARD_CAPTURE_HPP

#include <opencv2/opencv.hpp>

class Capture
{
    public:
    Capture(int width, int height, int fps, int device = 0);
    ~Capture();

    cv::Mat getFrame();

    private:
    cv::VideoCapture cap;
    cv::Mat frame;
};


#endif //OPENGUARD_CAPTURE_HPP
