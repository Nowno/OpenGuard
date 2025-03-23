#include "mog2.hpp"
#include <opencv2/opencv.hpp>
#include <json/json.hpp>

#include "../../openguard.hpp"
#include "../../hook_manager/hook_manager.hpp"

MOG2Detector::MOG2Detector()
{
    int width = ConfigManager::GetInstance().GetConfig<int>("frame_width");
    int height = ConfigManager::GetInstance().GetConfig<int>("frame_height");

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

    this->roi_selection = cv::Rect(0, 0, width, height);
    this->last_roi_selection = roi_selection;
}


/**
 * @brief Get bounding boxes from the foreground mask.
 * @param fgMask The foreground mask.
 * @return A vector of bounding boxes.
 */
std::vector<cv::Rect> MOG2Detector::GetMotionBB(const cv::Mat &fgMask)
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
    cv::Mat fg_mask;

    std::string on_motion_output = HookManager::GetInstance().GetHookOutput("on_motion", "roi_select");

    if (!on_motion_output.empty())
    {
        /// Parse the ROI selection
        auto roi_select = nlohmann::json::parse(on_motion_output);

        if (roi_select.contains("reset"))
        {
            this->roi_selection = cv::Rect(0, 0, frame.cols, frame.rows);
        }
        else
        {
            int x = roi_select["x"];
            int y = roi_select["y"];
            int width = roi_select["width"];
            int height = roi_select["height"];

            if (x >= 0 && y >= 0 && width > 0 && height > 0 && x + width <= frame.cols && y + height <= frame.rows)
            {
                std::cout << "Setting ROI selection" << std::endl;
                this->last_roi_selection = this->roi_selection;
                this->roi_selection = cv::Rect(x, y, width, height);
            }
            else
            {
                Logger::GetInstance().Log("ERROR", "Invalid ROI selection, resetting to full frame.");
                this->last_roi_selection = this->roi_selection;
                this->roi_selection = cv::Rect(0, 0, frame.cols, frame.rows);
            }
        }

        HookManager::GetInstance().ClearHookOutput("on_motion", "roi_select");
    }

    /// Draw the ROI selection
    overlay_renderer->Add(OverlayRenderer::OverlayElement(OverlayRenderer::DrawType::RECTANGLE, this->roi_selection, cv::Scalar(255, 25, 25), 4, false));


    /// Apply mog2 to the frame
    mog2->apply(frame, fg_mask);

    /// Edge case, sudden change in lighting upon initialization can cause false positives
    if (!initialized)
    {
        if (--initialization_frames == 0)
            initialized = true;
        return false;
    }

    cv::Mat fg_mask_roi = fg_mask(this->roi_selection);

    /// Preprocess the frame
    this->PreProcessFrame(fg_mask_roi);

    /// Count the number of non-zero pixels
    int motion_pixels = cv::countNonZero(fg_mask_roi);

    /// Render bounding boxes if enabled
    if (this->draw_bounding_boxes)
    {
        auto bounding_boxes = GetMotionBB(fg_mask);

        for (auto& box : bounding_boxes)
        {
            box.x += this->roi_selection.x;
            box.y += this->roi_selection.y;

            overlay_renderer->Add(OverlayRenderer::OverlayElement(OverlayRenderer::DrawType::RECTANGLE, box, cv::Scalar(0, 135, 135), 2));
        }
    }

    #ifdef DEBUG
        cv::imshow("Foreground Mask", fg_mask);
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