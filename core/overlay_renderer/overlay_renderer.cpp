//
// Created by Nono on 06/02/2025.
//

#include "overlay_renderer.hpp"
#include "../../util/utils.hpp"

OverlayRenderer::OverlayRenderer()
{
}

OverlayRenderer::~OverlayRenderer()
{

}

void OverlayRenderer::AddElement(OverlayElement element)
{
    this->elements.push_back(element);
}

void OverlayRenderer::Render(cv::Mat &frame, cv::Size size)
{
    static cv::Mat draw_overlay = cv::Mat::zeros(size.height, size.width, CV_8UC3);

    for (auto &element : elements)
    {
        switch (element.type)
        {
            case DrawType::RECTANGLE:
                cv::rectangle(frame, element.rect, element.color, element.thickness);
                break;
            case DrawType::CIRCLE:
                cv::circle(frame, element.position, element.radius, element.color, element.thickness);
                break;
            case DrawType::TEXT:
                cv::putText(frame, element.text, element.position, element.font_type, element.font_scale, element.color, element.thickness);
                break;
            case DrawType::LINE:
                cv::line(frame, element.position, element.rect.tl(), element.color, element.thickness);
                break;
            case DrawType::FILLED_RECTANGLE:
                cv::rectangle(frame, element.rect, element.color, cv::FILLED);
                break;
            case DrawType::FILLED_CIRCLE:
                cv::circle(frame, element.position, element.radius, element.color, cv::FILLED);
                break;
            case DrawType::IMAGE:
                cv::addWeighted(frame, element.alpha, overlay, 1.0 - element.alpha, 0, frame);
                break;
        }
    }

    cv::addWeighted(frame, 1.0, draw_overlay, 0.7, 0, frame);

    elements.clear();
}