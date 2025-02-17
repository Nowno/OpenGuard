#ifndef OPENGUARD_OVERLAY_RENDERER_HPP
#define OPENGUARD_OVERLAY_RENDERER_HPP
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
//todo: includes folder

class OverlayRenderer
{
    public:
    OverlayRenderer();
    ~OverlayRenderer();

    enum class DrawType
    {
        RECTANGLE,
        CIRCLE,
        TEXT,
        LINE,
        FILLED_RECTANGLE,
        FILLED_CIRCLE,
        IMAGE
    };


    struct OverlayElement {
        DrawType type;

        cv::Scalar color;
        cv::Point position;
        cv::Rect rect;

        int thickness;
        int radius;

        float alpha;

        std::string text;
        double font_scale;
        int font_type;

        bool persistent = false;

        // Constructor for basic shapes
        OverlayElement(DrawType t, cv::Scalar col, cv::Point pos, int thick = 1, bool persist = false)
                : type(t), color(col), position(pos), thickness(thick), alpha(1.0f), radius(0), font_scale(1.0), font_type(cv::FONT_HERSHEY_SIMPLEX), persistent(persist) {}

        OverlayElement(DrawType t, std::string txt, cv::Point pos, cv::Scalar col, double scale = 0.5, int thick = 1, int font = cv::FONT_HERSHEY_SIMPLEX, bool persist = false)
                : type(t), text(txt), position(pos), color(col), thickness(thick), alpha(1.0f), radius(0), font_scale(scale), font_type(font), persistent(persist) {}

        OverlayElement(DrawType t, cv::Rect r, cv::Scalar col, int thick = 1, bool persist = false)
                : type(t), rect(r), color(col), thickness(thick), alpha(1.0f), radius(0), font_scale(1.0), font_type(cv::FONT_HERSHEY_SIMPLEX), persistent(persist) {}

        OverlayElement(DrawType t, cv::Point center, int rad, cv::Scalar col, int thick = 1, bool persist = false)
                : type(t), position(center), radius(rad), color(col), thickness(thick), alpha(1.0f), font_scale(1.0), font_type(cv::FONT_HERSHEY_SIMPLEX), persistent(persist) {}

        OverlayElement(DrawType t, cv::Rect r, cv::Scalar col, float a, bool persist = false)
                : type(t), rect(r), color(col), thickness(-1), alpha(a), radius(0), font_scale(1.0), font_type(cv::FONT_HERSHEY_SIMPLEX), persistent(persist) {}    };


    void Add(const OverlayElement &element);
    void Render(cv::Mat& frame, cv::Size size);

    void InvalidatePersistent();
    bool IsPersistentInvalid() const;

    private:
    cv::Mat persistent_overlay;
    std::vector<OverlayElement> elements;
};



#endif //OPENGUARD_OVERLAY_RENDERER_HPP
