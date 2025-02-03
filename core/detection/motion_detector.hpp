#ifndef OPENGUARD_MOTION_DETECTOR_HPP
#define OPENGUARD_MOTION_DETECTOR_HPP

#include <opencv2/opencv.hpp>

class MotionDetector
{
    public:
    virtual ~MotionDetector() = default;
    virtual bool Detect(cv::Mat& frame) = 0;
};


#endif //OPENGUARD_MOTION_DETECTOR_HPP
 