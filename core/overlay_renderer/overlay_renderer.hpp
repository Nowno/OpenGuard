#ifndef OPENGUARD_OVERLAY_RENDERER_HPP
#define OPENGUARD_OVERLAY_RENDERER_HPP



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

    struct OverlayElement
    {
        DrawType type;
        cv::Scalar color;
        cv::Point start;
        cv::Point end;
        std::string text;
        int thickness;
        int radius;
        int font;
        int line_type;
        int shift;
        int image_id;
    };

};


#endif //OPENGUARD_OVERLAY_RENDERER_HPP
