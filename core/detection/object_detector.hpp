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
        NONE,
        PERSON,
        PET,
        CAR,
        OTHER
    };

    virtual ~ObjectDetector() = default;
    virtual Object Detect(const cv::Mat& frame) = 0;

    void setOverlayRenderer(std::shared_ptr<OverlayRenderer> renderer)
    {
        this->overlay_renderer = renderer;
    }

    std::string GetObjectString(Object object)
    {
        switch (object)
        {
            case Object::PERSON: return "Person";
            case Object::PET:    return "Pet";
            case Object::CAR:    return "Car";
            case Object::OTHER:  // Default (falthrough)
            default:             return "N/A";
        }
    }

    uint8_t GetDetectionCount() const
    {
        return detection_count;
    }

    void ResetDetectionCount()
    {
        detection_count = 0;
    }

    protected:
    std::shared_ptr<OverlayRenderer> overlay_renderer;
    uint8_t detection_count = 0;
};

#endif // OPENGUARD_OBJECT_DETECTOR_HPP
