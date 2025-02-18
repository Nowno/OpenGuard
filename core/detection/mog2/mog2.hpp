#include "../motion_detector.hpp"
#include <deque>
#ifndef OPENGUARD_MOG2_DETECTOR_HPP
#define OPENGUARD_MOG2_DETECTOR_HPP

class MOG2Detector : public MotionDetector
{
    private:
    //Basically a smart ptr.
    cv::Ptr<cv::BackgroundSubtractorMOG2> mog2;

    int motion_threshold;
    int initialization_frames;

    bool initialized;
    bool is_flickering;
    bool draw_bounding_boxes = false;
    bool motion_detected = false;

    double prev_brightness;

    bool LightFlickCheck(cv::Mat& frame);
    void PreProcessFrame(cv::Mat& frame);

    std::vector<cv::Rect> getMotionBB(const cv::Mat& fgMask);

    public:
    MOG2Detector(int threshold = 10000);
    bool Detect(cv::Mat& frame) override;
    void setDrawBoundingBoxes(bool draw);

};

#endif // OPENGUARD_MOG2_DETECTOR_HPP


//todo:params for mog2