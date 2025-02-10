#ifndef OPENGUARD_OBJECT_DETECTOR_HPP
#define OPENGUARD_OBJECT_DETECTOR_HPP

#include <opencv2/opencv.hpp>
#include <unordered_set>
#include "../overlay_renderer/overlay_renderer.hpp"

class ObjectDetector
{
    public:
    enum class Object
    {
        PERSON,
        PET,
        CAR,
        OTHER,
        NONE
    };

    virtual ~ObjectDetector() = default;
    virtual Object Detect(const cv::Mat& frame) = 0;

    void setOverlayRenderer(std::shared_ptr<OverlayRenderer> renderer)
    {
        this->overlay_renderer = renderer;
    }

    protected:
    //Initially used vector but this is more efficient for lookups
    std::unordered_set<Object> alert_objects;
    std::shared_ptr<OverlayRenderer> overlay_renderer;
};

#endif // OPENGUARD_OBJECT_DETECTOR_HPP
