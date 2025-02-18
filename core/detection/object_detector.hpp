#ifndef OPENGUARD_OBJECT_DETECTOR_HPP
#define OPENGUARD_OBJECT_DETECTOR_HPP

#include "../openguard.hpp"
#include "../overlay_renderer/overlay_renderer.hpp"

class ObjectDetector
{
    public:
    enum class Object
    {
        NONE,
        PERSON,
        PET,
        CAR
    };

    virtual ~ObjectDetector() = default;
    virtual Object Detect(const cv::Mat& frame) = 0;

    /**
     * @brief Set the overlay renderer.
     * @param renderer The overlay renderer.
     */
    void setOverlayRenderer(std::shared_ptr<OverlayRenderer> renderer) { this->overlay_renderer = renderer; }

    /**
     * @brief Get the string representation of an object.
     * @param object The object to get the string representation of.
     * @return The string representation of the object.
     */
    static std::string GetObjectString(Object object)
    {
        switch (object)
        {
            case Object::PERSON: return "person";
            case Object::PET:    return "pet";
            case Object::CAR:    return "car";
            default:             return "n/a";
        }
    }

    protected:
    std::shared_ptr<OverlayRenderer> overlay_renderer;
};

#endif // OPENGUARD_OBJECT_DETECTOR_HPP
