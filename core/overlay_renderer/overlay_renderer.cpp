//
// Created by Nono on 06/02/2025.
//

#include "overlay_renderer.hpp"
#include "../../utils/utils.hpp"

OverlayRenderer::OverlayRenderer()
{
}

OverlayRenderer::~OverlayRenderer()
{

}

void OverlayRenderer::Add(const OverlayElement& element)
{
    this->elements.push_back(element);
}

void OverlayRenderer::Render(cv::Mat &frame, cv::Size size)
{
    cv::Mat draw_overlay = cv::Mat::zeros(size.height, size.width, CV_8UC3);

    if (persistent_overlay.empty())
        persistent_overlay = cv::Mat::zeros(size.height, size.width, CV_8UC3);

    for (auto &element : elements)
    {
        auto overlay = element.persistent ? persistent_overlay : draw_overlay;

        switch (element.type)
        {
            case DrawType::RECTANGLE:
                cv::rectangle(overlay, element.rect, element.color, element.thickness);
                break;
            case DrawType::CIRCLE:
                cv::circle(overlay, element.position, element.radius, element.color, element.thickness);
                break;
            case DrawType::TEXT:
                cv::putText(overlay, element.text, element.position, element.font_type, element.font_scale, element.color, element.thickness);
                break;
            case DrawType::LINE:
                cv::line(overlay, element.position, element.rect.tl(), element.color, element.thickness);
                break;
            case DrawType::FILLED_RECTANGLE:
                cv::rectangle(overlay, element.rect, element.color, cv::FILLED);
                break;
            case DrawType::FILLED_CIRCLE:
                cv::circle(overlay, element.position, element.radius, element.color, cv::FILLED);
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
}

void OverlayRenderer::InvalidatePersistent()
{
    persistent_overlay = cv::Mat::zeros(persistent_overlay.size(), persistent_overlay.type());
}


bool OverlayRenderer::IsPersistentInvalid() const
{
    return persistent_overlay.empty() || cv::countNonZero(persistent_overlay) == 0;
}