#ifndef OPENGUARD_OVERLAY_RENDERER_HPP
#define OPENGUARD_OVERLAY_RENDERER_HPP
#include <vector>
#include <string>
#include <opencv2/opencv.hpp>

#include "../capture/capture.hpp"
//todo: includes folder

class OverlayRenderer
{
    public:
    OverlayRenderer(Capture& cap) : cap(cap) {}
    ~OverlayRenderer() {}

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


    struct OverlayElement{
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
        int line_type;

        bool persistent = false;

        // Constructor for basic shapes
        OverlayElement(DrawType t, cv::Scalar col, cv::Point pos, int thick = 1, bool persist = false, int _line_type = cv::LINE_8)
                : type(t), color(col), position(pos), thickness(thick), alpha(1.0f), radius(0), font_scale(1.0), font_type(cv::FONT_HERSHEY_SIMPLEX), persistent(persist), line_type(_line_type) {}

        // Constructor for text
        OverlayElement(DrawType t, std::string txt, cv::Point pos, cv::Scalar col, double scale = 0.5, int thick = 1, int font = cv::FONT_HERSHEY_SIMPLEX, bool persist = false, int _line_type = cv::LINE_8)
                : type(t), text(txt), position(pos), color(col), thickness(thick), alpha(1.0f), radius(0), font_scale(scale), font_type(font), persistent(persist), line_type(_line_type) {}

        // Constructor for rectangles
        OverlayElement(DrawType t, cv::Rect r, cv::Scalar col, int thick = 1, bool persist = false, int _line_type = cv::LINE_8)
                : type(t), rect(r), color(col), thickness(thick), alpha(1.0f), radius(0), font_scale(1.0), font_type(cv::FONT_HERSHEY_SIMPLEX), persistent(persist), line_type(_line_type) {}

        // Constructor for circles
        OverlayElement(DrawType t, cv::Point center, int rad, cv::Scalar col, int thick = 1, bool persist = false, int _line_type = cv::LINE_8)
                : type(t), position(center), radius(rad), color(col), thickness(thick), alpha(1.0f), font_scale(1.0), font_type(cv::FONT_HERSHEY_SIMPLEX), persistent(persist), line_type(_line_type) {}

        // Constructor for filled shapes
        OverlayElement(DrawType t, cv::Rect r, cv::Scalar col, float a, bool persist = false, int _line_type = cv::LINE_8)
                : type(t), rect(r), color(col), thickness(-1), alpha(a), radius(0), font_scale(1.0), font_type(cv::FONT_HERSHEY_SIMPLEX), persistent(persist), line_type(_line_type) {}
    };


    void Add(const OverlayElement &element);
    void Render(cv::Mat& frame, cv::Size size);

    void InvalidatePersistent();
    bool IsPersistentInvalid() const { return persistent_invalid; }

    void SetRenderHUD(bool render) { render_hud = render; }
    void SetRenderDebug(bool render) { render_debug = render; }

    private:
    void RenderHUD(cv::Size size);

    cv::Mat persistent_overlay;
    bool render_hud = true;
    bool render_debug = true;
    bool persistent_invalid = true;
    std::vector<OverlayElement> elements;

    Capture& cap;
};



#endif //OPENGUARD_OVERLAY_RENDERER_HPP
