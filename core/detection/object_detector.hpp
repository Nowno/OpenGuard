#ifndef OPENGUARD_OBJECT_DETECTOR_HPP
#define OPENGUARD_OBJECT_DETECTOR_HPP

#include <opencv2/opencv.hpp>
#include "../capture/capture.hpp"
#include <unordered_set>

class ObjectDetector
{
    public:
    enum class Object
    {
        PERSON,
        PET,
        OTHER,
        NONE
    };

    virtual ~ObjectDetector() = default;

    virtual Object Detect(const cv::Mat& frame) = 0;

    void SetAlertObjects(const std::vector<Object>& objects)
    {
        alert_objects.clear();
        alert_objects.insert(objects.begin(), objects.end());
    }

    bool ShouldAlert(Object detectedObject) const
    {
        return alert_objects.find(detectedObject) != alert_objects.end();
    }

    protected:
    //Initially used vector but this is more efficient for lookups
    std::unordered_set<Object> alert_objects;
};

#endif // OPENGUARD_OBJECT_DETECTOR_HPP
