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
    //This is the value used to mark shadows in the foreground mask.
    mog2->setDetectShadows(true);
    mog2->setShadowValue(127);

    mog2->setVarThreshold(10);              // Lower threshold = more sensitivity
    mog2->setHistory(300);                  // Number of frames to keep in memory
    mog2->setShadowThreshold(0.5); // Reduce false positives from shadowsq
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

void MOG2Detector::PreProcessFrame(cv::Mat& frame)
{
    //https://docs.opencv.org/3.4/d9/d61/tutorial_py_morphological_ops.html
    auto kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));

    //Remove small noise
    cv::morphologyEx(frame, frame, cv::MORPH_OPEN, kernel);

    //Fill the holes in the object produces by the previous call
    cv::morphologyEx(frame, frame, cv::MORPH_CLOSE, kernel);

}


bool MOG2Detector::Detect(cv::Mat& frame)
{
    cv::Mat fgMask;
    mog2->apply(frame, fgMask);

    if (!initialized)
    {
        if (--initialization_frames == 0)
            initialized = true;
        return false;
    }

    PreProcessFrame(fgMask); // Remove noise

    int motionThreshold = frame.cols * frame.rows * 0.0025; // 0.25% of pixels

    int motionPixels = cv::countNonZero(fgMask);

    cv::imshow("Foreground Mask", fgMask); // Debugging

    return motionPixels > motionThreshold;
}


void MOG2Detector::setDrawBoundingBoxes(bool draw)
{
    this->draw_bounding_boxes = draw;
}