#include "../motion_detector.hpp"
#include <deque>
#ifndef OPENGUARD_MOG2_DETECTOR_HPP
#define OPENGUARD_MOG2_DETECTOR_HPP

class MOG2Detector : public MotionDetector
{
    public:
    /**
     * @brief Constructor: Initializes the MOG2 detector.
     */
    MOG2Detector();

    /**
     * @brief Detect motion in a frame.
     * @param frame The input image to detect motion in.
     * @return Whether motion was detected.
     */
    bool Detect(cv::Mat& frame) override;

    /**
     * @brief Set whether to draw bounding boxes or not.
     * @param draw if true, bounding boxes will be drawn.
     */
    void setDrawBoundingBoxes(bool draw);


    private:
    //Basically a smart ptr.
    cv::Ptr<cv::BackgroundSubtractorMOG2> mog2;

    cv::Rect roi_selection;
    cv::Rect last_roi_selection;

    int motion_threshold;      /// The threshold for motion detection
    int initialization_frames; /// Number of frames to ignore

    bool initialized = false;
    bool draw_bounding_boxes = false;

    /**
     * @brief Preprocesses the input frame, adding morphological operations.
     */
    void PreProcessFrame(cv::Mat& frame);

    /**
     * @brief Get bounding boxes from the foreground mask.
     * @param fgMask The foreground mask.
     * @return A vector of bounding boxes.
     */
    std::vector<cv::Rect> GetMotionBB(const cv::Mat& fgMask);
};

#endif // OPENGUARD_MOG2_DETECTOR_HPP
