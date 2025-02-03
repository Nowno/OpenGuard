#ifndef OPENGUARD_YOLO_HPP
#define OPENGUARD_YOLO_HPP

#include "../object_detector.hpp"
#include <opencv2/dnn.hpp>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

class YOLODetector : public ObjectDetector
{
    public:
    YOLODetector(const std::string& modelPath, const std::string& configPath, const std::string& classesPath);
    ~YOLODetector() override = default;

    Object Detect(const cv::Mat& frame) override;

    private:
    cv::dnn::Net net;
    std::vector<std::string> classNames;
    float confidenceThreshold;
    float nmsThreshold;

    void LoadClassNames(const std::string& classesPath);
    std::vector<cv::Rect> PostProcess(const cv::Mat& frame, const std::vector<cv::Mat>& outputs);
};

#endif // OPENGUARD_YOLO_HPP
