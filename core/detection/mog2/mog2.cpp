#include "mog2.hpp"
#include <opencv2/opencv.hpp>
#include <iostream>

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



bool MOG2Detector::LightFlickCheck(cv::Mat& frame)
{/*
 * std::deque<double> brightnessHistory;  // Stores the last N brightness values
const int N = 5;  // Number of frames to track
const double flickerThreshold = 30.0;  // Base threshold (adjust dynamically)
    cv::Scalar avg_brightness = cv::mean(frame);
    double currentBrightness = avg_brightness[0];

    if (brightnessHistory.size() >= N)
        brightnessHistory.pop_front();  // Remove oldest brightness value

    brightnessHistory.push_back(currentBrightness);

    if (brightnessHistory.size() < N)
        return false;  // Not enough frames yet

    // ✅ Step 1: Compute Brightness Drop (Oldest vs. Current Frame)
    double brightnessDrop = brightnessHistory.front() - currentBrightness;  // Compare oldest frame in history

    // ✅ Step 2: Ignore motion-based brightness changes
    cv::Mat fgMask;
    mog2->apply(frame, fgMask);
    int motionPixels = cv::countNonZero(fgMask);
    double motionPercentage = (double)motionPixels / (fgMask.rows * fgMask.cols) * 100.0;

    std::cout << "Brightness Drop: " << brightnessDrop
              << " | Motion %: " << motionPercentage << std::endl;

    // ✅ Step 3: Trigger Flicker Detection ONLY if brightness dropped significantly & no major motion
    if (brightnessDrop > 60 && motionPercentage < 5.0)  // Adjust threshold as needed
    {
        std::cout << "⚠️ LIGHT FLICKER DETECTED (Lights turned OFF)" << std::endl;
        return true;
    }*/

    return false;
}


std::vector<cv::Rect> MOG2Detector::GetMotionBB(const cv::Mat &fgMask)
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
    if (!initialized)
    {
        if (--initialization_frames == 0)
            initialized = true;

        return false;
    }

    PreprocessFrame(frame);

    if (LightFlickCheck(frame))
        return false;

    cv::Mat fgMask;
    mog2->apply(frame, fgMask);

    int motionPixels = cv::countNonZero(fgMask);

    if (this->draw_bounding_boxes)
    {
        std::vector<cv::Rect> motionBoxes = GetMotionBB(fgMask);

        for (const auto& box : motionBoxes)
            cv::rectangle(frame, box, cv::Scalar(0, 255, 0), 2);
    }

    return motionPixels > motion_threshold;
}
