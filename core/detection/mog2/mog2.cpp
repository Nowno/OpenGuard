#include "mog2.hpp"
#include <opencv2/opencv.hpp>
#include "../../openguard.hpp"

MOG2Detector::MOG2Detector(int threshold)
{
    this->motion_threshold = threshold;
    this->initialization_frames = 30;
    this->initialized = false;

    mog2 = cv::createBackgroundSubtractorMOG2();

    //Shadow detection
    mog2->setDetectShadows(true);
    //This is the value used to mark shadows in the foreground mask.
    mog2->setShadowValue(127);
}


//Deprecated by yolov5
bool MOG2Detector::LightFlickCheck(cv::Mat& frame)
{
    return false;
}


std::vector<cv::Rect> MOG2Detector::getMotionBB(const cv::Mat &fgMask)
{
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Rect> bounding_boxes;

    cv::findContours(fgMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for (const auto& contour : contours)
    {
        cv::Rect box = cv::boundingRect(contour);

        if (box.area() > 500)
            bounding_boxes.push_back(box);
    }

    return bounding_boxes;
}

void MOG2Detector::PreprocessFrame(cv::Mat& frame)
{
    // reduce noise before thresholding
    cv::GaussianBlur(frame, frame, cv::Size(5, 5), 0);
}

bool MOG2Detector::Detect(cv::Mat& frame)
{
    cv::Mat fgMask;
    mog2->apply(frame, fgMask);

    if (!initialized)
    {
        if (--initialization_frames == 0)
            initialized = true;

        //Todo print

        return false;
    }

    PreprocessFrame(fgMask);

    int motionPixels = cv::countNonZero(fgMask);

    if (this->draw_bounding_boxes)
    {
        std::vector<cv::Rect> motionBoxes = getMotionBB(fgMask);

        for (const auto& box : motionBoxes)
            overlay_renderer->Add(OverlayRenderer::OverlayElement(OverlayRenderer::DrawType::RECTANGLE, box, cv::Scalar(0, 255, 0), 2));
    }

    prev_state = motion_detected;
    motion_detected = motionPixels > motion_threshold;

    return motion_detected;
}

void MOG2Detector::setDrawBoundingBoxes(bool draw)
{
    this->draw_bounding_boxes = draw;
}