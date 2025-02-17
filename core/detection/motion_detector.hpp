#ifndef OPENGUARD_MOTION_DETECTOR_HPP
#define OPENGUARD_MOTION_DETECTOR_HPP

#include <opencv2/opencv.hpp>
#include "../overlay_renderer/overlay_renderer.hpp"

class MotionDetector
{
    public:
    virtual ~MotionDetector() = default;
    virtual bool Detect(cv::Mat& frame) = 0;

    void setOverlayRenderer(std::shared_ptr<OverlayRenderer> renderer)
    {
        this->overlay_renderer = renderer;
    }

    bool GetPreviousState()
    {
        return prev_state;
    }

    protected:
    std::shared_ptr<OverlayRenderer> overlay_renderer;
    bool prev_state = false;
};


#endif //OPENGUARD_MOTION_DETECTOR_HPP
 