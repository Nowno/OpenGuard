#ifndef OPENGUARD_YOLO_HPP
#define OPENGUARD_YOLO_HPP

#include <opencv2/dnn.hpp>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

#include "../object_detector.hpp"
#include "../../../utils/worker_thread/worker_thread.hpp"

class YOLODetector : public ObjectDetector
{
    public:
    YOLODetector();
    ~YOLODetector();

    /**
     * @brief Detect objects in a frame.
     * @param frame The input image to detect objects in.
     * @return What object was detected.
     */
    Object Detect(const cv::Mat& frame) override;

    /**
     * @brief Enable or disable hardware acceleration.
     * @param use_gpu Whether to use GPU or not.
     */
    void setHardwareAcceleration(bool use_gpu);

    /**
     * @brief Set whether to draw bounding boxes or not.
     * @param draw Whether to draw bounding boxes or not.
     */
    void setDrawBoundingBoxes(bool draw);

    private:
    /**
    * @brief Struct to simplify storing detection results.
    */
    struct DetectionResult
    {
        std::vector<cv::Rect> boxes;    /// Boundind boxes of detected objects.
        std::vector<int> class_ids;     /// Class IDs of detected objects.
        std::vector<float> confidences; /// Confidence for each detected object.
    };

    cv::dnn::Net net;                     /// The neural network
    std::vector<std::string> class_names; /// All the object classes

    float confidence_threshold; /// Minimum it takes to consider an object detected
    float score_threshold;      /// Minimum class score
    float nms_threshold;        /// Non-maximum suppression threshold

    bool draw_bounding_boxes = true;

    int yolo_resolution;
    int width;
    int call_count;

    std::unique_ptr<WorkerThread<cv::Mat, DetectionResult>> worker_thread; /// Worker thread to run detection on
    Object last_detection;
    std::mutex detection_mutex;                                            /// Mutex to protect the detection result


    bool RetrieveInferenceResults(DetectionResult& result);

    DetectionResult RunInference(const cv::Mat& frame);

    /**
     * @brief Load class names from a file.
     * @param classesPath The path to the file containing the class names.
     * @return A vector containing the class names.
     */
    void LoadClassNames(const std::string& classesPath);

    /**
     * @brief Pre-process the input image, in our case we resize it to 640x640.
     * @param frame The input image.
     * @return The resized image.
     */
    cv::Mat PreProcess(const cv::Mat& frame);

    /**
     * @brief Post-process the output of the neural network.
     * @param frame The input image.
     * @param outputs The output of the neural network.
     * @return All the detected objects and their bounding boxes.
     */
    DetectionResult PostProcess(const cv::Mat& frame, const std::vector<cv::Mat>& outputs);

    /**
     * @brief Draw bounding boxes on the frame.
     * @param result The detection result.
     */
    void DrawBoundingBoxes(const DetectionResult& result);


};

#endif // OPENGUARD_YOLO_HPP
