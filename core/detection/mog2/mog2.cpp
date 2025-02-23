#include "mog2.hpp"
#include <opencv2/opencv.hpp>
#include "../../openguard.hpp"

MOG2Detector::MOG2Detector()
{
    int width = ConfigManager::GetInstance().GetConfig<int>("frame_width");
    int height = ConfigManager::GetInstance().GetConfig<int>("frame_width");

    /// Calculate the motion threshold based on the number of pixels
    this->motion_threshold =  width * height * (ConfigManager::GetInstance().GetConfig<float>("mog2_motion_threshold") / 100.0f);

    /// Number of frames to ignore
    this->initialization_frames = ConfigManager::GetInstance().GetConfig<int>("frame_rate") * 2;

    /// Create the MOG2 background subtractor
    mog2 = cv::createBackgroundSubtractorMOG2();

    /// Shadow detection
    mog2->setDetectShadows(ConfigManager::GetInstance().GetConfig<bool>("mog2_detect_shadows"));
    mog2->setShadowValue(127);
    mog2->setShadowThreshold(0.5);  /// Reduce false positives from shadows

    /// Sensitivity
    mog2->setVarThreshold(ConfigManager::GetInstance().GetConfig<int>("mog2_sensitivity"));    /// Lower threshold = more sensitivity
    mog2->setHistory(ConfigManager::GetInstance().GetConfig<int>("mog2_history"));             /// Number of frames to keep in memory
}


/**
 * @brief Get bounding boxes from the foreground mask.
 * @param fgMask The foreground mask.
 * @return A vector of bounding boxes.
 */
std::vector<cv::Rect> MOG2Detector::getMotionBB(const cv::Mat &fgMask)
{
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Rect> bounding_boxes;

    cv::findContours(fgMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for (const auto& contour : contours)
    {
        cv::Rect box = cv::boundingRect(contour);

        /// Small triage to remove small noise
        if (box.area() > 500)
            bounding_boxes.push_back(box);
    }

    return bounding_boxes;
}

/**
 * @brief Preprocesses the input frame, adding morphological operations.
 */
void MOG2Detector::PreProcessFrame(cv::Mat& frame)
{
    /// https://docs.opencv.org/3.4/d9/d61/tutorial_py_morphological_ops.html
    auto kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));

    /// Remove small noise
    cv::morphologyEx(frame, frame, cv::MORPH_OPEN, kernel);

    /// Fill the holes in the object produces by the previous call
    cv::morphologyEx(frame, frame, cv::MORPH_CLOSE, kernel);
}


bool MOG2Detector::Detect(cv::Mat& frame)
{
    cv::Mat fgMask;

    /// Apply mog2 to the frame
    mog2->apply(frame, fgMask);

    /// Edge case, sudden change in lighting upon initialization can cause false positives
    if (!initialized)
    {
        if (--initialization_frames == 0)
            initialized = true;
        return false;
    }

    /// Preprocess the frame
    this->PreProcessFrame(fgMask);

    /// Count the number of non-zero pixels
    int motion_pixels = cv::countNonZero(fgMask);

    /// Render bounding boxes if enabled
    if (this->draw_bounding_boxes)
    {
        auto bounding_boxes = getMotionBB(fgMask);

        for (const auto& box : bounding_boxes)
        {
            overlay_renderer->Add(OverlayRenderer::OverlayElement(OverlayRenderer::DrawType::RECTANGLE, box, cv::Scalar(0, 255, 0), 2));
        }
    }

    #ifdef DEBUG
        cv::imshow("Foreground Mask", fgMask);
    #endif

    /// Return whether the amount of motion exceeds the threshold
    return motion_pixels > this->motion_threshold;
}

/**
 * @brief Set whether to draw bounding boxes or not.
 */
void MOG2Detector::setDrawBoundingBoxes(bool draw)
{
    this->draw_bounding_boxes = draw;
}