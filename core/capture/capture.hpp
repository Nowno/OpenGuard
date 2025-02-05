#ifndef OPENGUARD_CAPTURE_HPP
#define OPENGUARD_CAPTURE_HPP
//todo: optimise imports
#include <opencv2/opencv.hpp>
#include "../../util/utils.hpp"

class Capture
{
    public:
    Capture(int width, int height, int fps, int device = 0);
    ~Capture();

    cv::Mat getFrame();
    cv::VideoCapture getCapture();

    Vec2 getFrameSize();
    int getFrameCount();
    int getFPS();

    void setFPS(int fps);
    void setResolution(Vec2 resolution);
    void Update();


    private:
    cv::VideoCapture cap;
    cv::Mat frame;

    int frame_count = 0;
    int frame_counter = 0;
    int fps = 0;
};


#endif //OPENGUARD_CAPTURE_HPP
