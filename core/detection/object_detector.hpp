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

    /**
     * @brief Detect objects in a frame.
     * @param frame The input image to detect objects in.
     * @return What object was detected.
     */
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
            case Object::NONE:   return "none";
            default:             return "none";
        }
    }

    /**
     * @brief The opposite of the previous function, gets the object from a string.
     * @param object The string representation of the object.
     * @return The object (enum).
     */
    static Object GetObjectFromString(const std::string& object)
    {
        if (object == "person")
            return Object::PERSON;
        else if (object == "cat" || object == "dog")
            return Object::PET;
        else if (object == "car" || object == "truck")
            return Object::CAR;
        else
            return Object::NONE;
    }

    protected:
    std::shared_ptr<OverlayRenderer> overlay_renderer;
};

#endif // OPENGUARD_OBJECT_DETECTOR_HPP
