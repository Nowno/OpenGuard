#ifndef OPENGUARD_MOTION_DETECTOR_HPP
#define OPENGUARD_MOTION_DETECTOR_HPP

#include <opencv2/opencv.hpp>
#include "../overlay_renderer/overlay_renderer.hpp"

class MotionDetector
{
    public:
    virtual ~MotionDetector() = default;

    /**
     * @brief Detect motion in a frame.
     * @param frame The input image to detect motion in.
     * @return Whether motion was detected.
     */
    virtual bool Detect(cv::Mat& frame) = 0;

    /**
    * @brief Set the overlay renderer.
    * @param renderer The overlay renderer.
    */
    void setOverlayRenderer(std::shared_ptr<OverlayRenderer> renderer) { this->overlay_renderer = renderer;}

    protected:
    std::shared_ptr<OverlayRenderer> overlay_renderer;
};


#endif //OPENGUARD_MOTION_DETECTOR_HPP
 