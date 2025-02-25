#include "yolo.hpp"
#include "../../openguard.hpp"

#include <stdexcept>
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <fstream>

/**
 * Self-Implemented based on:
 * https://learnopencv.com/object-detection-using-yolov5-and-opencv-dnn-in-c-and-python/
 *
 */


/**
 * @brief Constructor: Initializes YOLOv5 detector.
 */
YOLODetector::YOLODetector()
{
    /// Load YOLOv5 model
    net = cv::dnn::readNetFromONNX(ConfigManager::GetInstance().GetConfig<std::string>("yolo_model_path"));

    if (net.empty())
    {
        Logger::GetInstance().Log("FATAL", "Could not load YOLOv5 model.");
        throw std::runtime_error("Could not load YOLOv5 model.");
    }

    /// Load class names
    LoadClassNames(ConfigManager::GetInstance().GetConfig<std::string>("yolo_classes_path"));

    /// Read detection thresholds
    confidence_threshold = ConfigManager::GetInstance().GetConfig<float>("yolo_confidence_threshold");
    score_threshold = ConfigManager::GetInstance().GetConfig<float>("yolo_score_threshold");
    yolo_resolution = ConfigManager::GetInstance().GetConfig<int>("yolo_resolution");

    nms_threshold = 0.45; /// Decided against exposing this to the user as it may not be intuitive

    /// Set preferable backend & target (CPU or CUDA if available)
    setHardwareAcceleration(ConfigManager::GetInstance().GetConfig<bool>("use_gpu"));

    worker_thread = std::make_unique<WorkerThread<cv::Mat, DetectionResult>>
            ([this](const cv::Mat &frame) -> DetectionResult
             {
                 return RunInference(frame);
             });

    worker_thread->Start();
}

/**
 * @brief Destructor: Joins worker thread to avoid leaks
 */
YOLODetector::~YOLODetector()
{
    if (worker_thread)
        worker_thread->Stop();
}

/**
 * @brief Loads class names from file.
 */
void YOLODetector::LoadClassNames(const std::string &classesPath)
{
    std::ifstream file(classesPath);
    if (!file.is_open())
    {
        throw std::runtime_error("Could not open class names file: " + classesPath);
    }

    std::string line;
    while (std::getline(file, line))
        class_names.push_back(line);

    file.close();
}

/**
 * @brief Enable or disable hardware acceleration.
 */
void YOLODetector::setHardwareAcceleration(bool use_gpu)
{
    if (use_gpu && cv::cuda::getCudaEnabledDeviceCount() > 0)
    {
        net.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
        net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
    }
    else
    {
        if (use_gpu)
        {
            Logger::GetInstance().Log("WARNING", "CUDA not available, falling back to CPU.");
        }

        net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
    }
}

/**
 * @brief Set whether to draw bounding boxes or not.
 */
void YOLODetector::setDrawBoundingBoxes(bool draw)
{
    this->draw_bounding_boxes = draw;
}

/**
 * @brief Preprocesses the input frame.
 */
cv::Mat YOLODetector::PreProcess(const cv::Mat &frame)
{
    int max_dim = std::max(frame.cols, frame.rows);
    cv::Mat padded;

    /// To avoid deforming the image, pad it to a square.
    cv::copyMakeBorder(frame, padded, (max_dim - frame.rows) / 2, (max_dim - frame.rows) / 2, (max_dim - frame.cols) / 2, (max_dim - frame.cols) / 2, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
    cv::resize(padded, padded, cv::Size(yolo_resolution, yolo_resolution));

    return padded;
}

void YOLODetector::DrawBoundingBoxes(const YOLODetector::DetectionResult &result)
{
    /// Draw detections on frame
    for (size_t i = 0; i < result.boxes.size(); i++)
    {
        cv::Rect box = result.boxes[i];
        int class_id = result.class_ids[i];
        float confidence = result.confidences[i];

        std::string label = class_names[class_id] + " " + std::to_string(confidence);

        overlay_renderer->Add(OverlayRenderer::OverlayElement(OverlayRenderer::DrawType::RECTANGLE, box, cv::Scalar(0, 255, 0), 2, true));
        overlay_renderer->Add(OverlayRenderer::OverlayElement(OverlayRenderer::DrawType::TEXT, label, cv::Point(box.x,box.y - 10), cv::Scalar(0, 255, 0), 2, true, cv::FONT_HERSHEY_SIMPLEX, true));
    }
}

/**
 * @brief Processes YOLO output and extracts detections.
 */
YOLODetector::DetectionResult YOLODetector::PostProcess(const cv::Mat& frame, const std::vector<cv::Mat>& outputs)
{
    std::vector<cv::Rect> boxes;
    std::vector<int> class_ids;
    std::vector<float> confidences;

    float x_factor = static_cast<float>(frame.cols) / yolo_resolution;
    float y_factor = static_cast<float>(frame.rows) / yolo_resolution;

    if (outputs.empty())
    {
        Logger::GetInstance().Log("ERROR", "PostProcess received empty outputs.");
        return {};
    }

    cv::Mat output = outputs[0];

    if (output.empty())
    {
        Logger::GetInstance().Log("ERROR", "YOLO output is empty.");
        return {};
    }

    /// Ensure correct reshaping (if the output is 3D, reshape to 2D)
    if (output.dims == 3 && output.size[0] == 1)
        output = output.reshape(1, output.size[1]);

    for (int i = 0; i < output.rows; i++)
    {
        /// Extract bounding box coordinates
        float confidence = output.at<float>(i, 4);
        if (confidence > confidence_threshold)
        {
            /// Extract class scores
            cv::Mat scores = output.row(i).colRange(5, output.cols);
            cv::Point class_id_point;
            double max_class_score;

            cv::minMaxLoc(scores, nullptr, &max_class_score, nullptr, &class_id_point);

            /// Proceed if class score is above threshold
            if (max_class_score > score_threshold)
            {
                /// Check if the class is supported, omit it otherwise.
                const static std::unordered_set<std::string> supported_classes = {"person", "car", "truck", "cat", "dog"};

                std::string class_name = class_names[class_id_point.x];

                if (supported_classes.find(class_name) == supported_classes.end())
                    continue;

                /// center x, center y, width, height
                float cx = output.at<float>(i, 0);
                float cy = output.at<float>(i, 1);
                float w = output.at<float>(i, 2);
                float h = output.at<float>(i, 3);

                /// Convert to pixel coordinates, multiply by factor to scale to original frame size
                int left = std::max(0, static_cast<int>((cx - 0.5f * w) * x_factor));
                int top = std::max(0, static_cast<int>((cy - 0.5f * h) * y_factor));

                /// Ensure bounding box is within frame
                int width = std::min(static_cast<int>(w * x_factor), frame.cols - left);
                int height = std::min(static_cast<int>(h * y_factor), frame.rows - top);

                /// Add bounding box to list
                boxes.emplace_back(left, top, width, height);
                confidences.push_back(static_cast<float>(max_class_score));
                class_ids.push_back(class_id_point.x);
            }
        }
    }

    /// Apply Non-Maximum Suppression (NMS)
    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, confidences, score_threshold, nms_threshold, indices);

    DetectionResult result;

    /// NMS removes the boxes with lower confidence, so we only keep the ones that are left
    for (int idx : indices)
    {
        if (idx >= boxes.size())
        {
            Logger::GetInstance().Log("ERROR", "NMS index out of range.");
            continue;
        }

        result.boxes.push_back(boxes[idx]);
        result.class_ids.push_back(class_ids[idx]);
        result.confidences.push_back(confidences[idx]);
    }

    return result;
}


bool YOLODetector::RetrieveInferenceResults(YOLODetector::DetectionResult &result)
{
    std::lock_guard<std::mutex> lock(detection_mutex);

    if (worker_thread->GetResult(result))
        return true;

    return false;
}

YOLODetector::DetectionResult YOLODetector::RunInference(const cv::Mat &frame)
{
    /// Preprocess the frame
    cv::Mat resized = PreProcess(frame);
    cv::Mat blob = cv::dnn::blobFromImage(resized, 1 / 255.0, cv::Size(yolo_resolution, yolo_resolution), cv::Scalar(0, 0, 0), true, false);

    /// Ensure network input is valid
    if (blob.empty())
    {
        Logger::GetInstance().Log("ERROR", "YOLO blob is empty.");
        return {};
    }

    net.setInput(blob);
    std::vector<cv::Mat> outputs;

    /// Forward pass
    net.forward(outputs, net.getUnconnectedOutLayersNames());

    if (outputs.empty())
    {
        Logger::GetInstance().Log("ERROR", "YOLO outputs are empty.");
        return {};
    }

    /// Post-process the output, extract detections and return them
    return PostProcess(frame, outputs);
}

/**
 * @brief Runs YOLO detection on a given frame.
 */

ObjectDetector::Object YOLODetector::Detect(const cv::Mat &frame)
{
    static int call_freq = ConfigManager::GetInstance().GetConfig<int>("object_detection_frequency");

    if (call_freq > 1 && call_count++ % call_freq != 0)
        return last_detection;

    /// If the queue is full, don't add more jobs to avoid delaying the detection too much
    {
        std::lock_guard<std::mutex> lock(detection_mutex);
        if (worker_thread->GetQueueSize() > 5)
            return last_detection;

        worker_thread->AddJob(frame.clone());
    }

    DetectionResult result; /// Will hold with the inference results

    if (RetrieveInferenceResults(result))
    {
        std::lock_guard<std::mutex> lock(detection_mutex);

        if (!result.class_ids.empty()) /// Make sure something was written to the result
        {
            last_detection = ObjectDetector::GetObjectFromString(class_names[result.class_ids[0]]);

            if (this->draw_bounding_boxes)
            {
                overlay_renderer->InvalidatePersistent();
                DrawBoundingBoxes(result);
            }
        }
        else
        {
            last_detection = ObjectDetector::Object::NONE;
        }
    }
    else
    {
        last_detection = ObjectDetector::Object::NONE;
    }

    return last_detection;
}