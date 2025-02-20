#include "overlay_renderer.hpp"

void OverlayRenderer::InvalidatePersistent()
{
     persistent_overlay = cv::Mat::zeros(persistent_overlay.size(), persistent_overlay.type());
     persistent_invalid = true;
}

void OverlayRenderer::Add(const OverlayElement& element)
{
    this->elements.push_back(element);
}

void OverlayRenderer::RenderHUD(cv::Size size)
{
    if (this->render_debug)
    {
        Add(OverlayElement(DrawType::TEXT, std::to_string(cap.GetFPS()) + " FPS", cv::Point(size.width - 110, 20), cv::Scalar(255, 255, 255), 0.4, 1, cv::FONT_HERSHEY_SIMPLEX, false));
        Add(OverlayElement(DrawType::TEXT, std::to_string(cap.GetFrameTime()) + " ms", cv::Point(size.width - 60, 20), cv::Scalar(255, 255, 255), 0.4, 1, cv::FONT_HERSHEY_SIMPLEX, false));
    }

    if (this->persistent_invalid)
    {
        Add(OverlayElement(DrawType::CIRCLE, cv::Point(20, 20), 8, cv::Scalar(0, 0, 255), cv::FILLED, true, cv::LINE_AA));
        Add(OverlayElement(DrawType::TEXT, "REC", cv::Point(32, 26), cv::Scalar(255, 255, 255), 0.6, 1, cv::FONT_HERSHEY_SIMPLEX, true, cv::LINE_AA));
    }

    Add(OverlayElement(DrawType::TEXT, OpenGuard::Utils::GetDateTimeString(false), cv::Point(size.width - 150, size.height - 10), cv::Scalar(255, 255, 255), 0.4, 1, cv::FONT_HERSHEY_SIMPLEX, false));
}

void OverlayRenderer::Render(cv::Mat &frame, cv::Size size)
{
    cv::Mat draw_overlay = cv::Mat::zeros(size.height, size.width, CV_8UC3);

    if (persistent_overlay.empty())
        persistent_overlay = cv::Mat::zeros(size.height, size.width, CV_8UC3);

    if (this->render_hud)
        RenderHUD(size);

    for (auto &element : elements)
    {
        auto overlay = element.persistent ? persistent_overlay : draw_overlay;

        switch (element.type)
        {
            case DrawType::RECTANGLE:
                cv::rectangle(overlay, element.rect, element.color, element.thickness, element.line_type);
                break;
            case DrawType::CIRCLE:
                cv::circle(overlay, element.position, element.radius, element.color, element.thickness, element.line_type);
                break;
            case DrawType::TEXT:
                cv::putText(overlay, element.text, element.position, element.font_type, element.font_scale, element.color, element.thickness, element.line_type);
                break;
            case DrawType::LINE:
                cv::line(overlay, element.position, element.rect.tl(), element.color, element.thickness, element.line_type);
                break;
            case DrawType::FILLED_RECTANGLE:
                cv::rectangle(overlay, element.rect, element.color, cv::FILLED, element.line_type);
                break;
            case DrawType::FILLED_CIRCLE:
                cv::circle(overlay, element.position, element.radius, element.color, cv::FILLED, element.line_type);
                break;
            case DrawType::IMAGE:
                //cv::addWeighted(draw_overlay, element.alpha, overlay, 1.0 - element.alpha, 0, frame);
                break;
        }

    }

    //add persistent overlay to frame
    cv::addWeighted(frame, 1.0, persistent_overlay, 0.7, 0, frame);
    cv::addWeighted(frame, 1.0, draw_overlay, 0.7, 0, frame);

    elements.clear();
    persistent_invalid = false;
}