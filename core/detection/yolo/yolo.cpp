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

//todo: experiment with yolov5n instead of yolov5s
//todo: yoloV8 +focus only few classes when exporting model

/**
 * @brief Constructor: Initializes YOLOv5 detector.
 */
YOLODetector::YOLODetector()
{
    // Load YOLOv5 model
    net = cv::dnn::readNetFromONNX(ConfigManager::GetInstance().GetConfig<std::string>("model_path"));
    if (net.empty())
    {
        throw std::runtime_error("Could not load YOLOv5 model.");
    }

    // Load class names
    LoadClassNames(ConfigManager::GetInstance().GetConfig<std::string>("classes_path"));

    // Read detection thresholds
    confidence_threshold = std::stof(ConfigManager::GetInstance().GetConfig<std::string>("confidence_threshold"));
    score_threshold = std::stof(ConfigManager::GetInstance().GetConfig<std::string>("score_threshold"));
    nms_threshold = std::stof(ConfigManager::GetInstance().GetConfig<std::string>("nms_threshold"));
    // Set preferable backend & target (CPU or CUDA if available)
    setHardwareAcceleration(ConfigManager::GetInstance().GetConfig<bool>("use_gpu"));
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
    if (use_gpu)
    {
        net.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
        net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
    }
    else
    {
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
    cv::Mat resized;
    cv::resize(frame, resized, cv::Size(640, 640)); // Ensuring correct YOLOv5 input size
    return resized;
}

void YOLODetector::DrawBoundingBoxes(cv::Mat &frame, const YOLODetector::DetectionResult &result)
{        //            overlay_renderer->Add(OverlayRenderer::OverlayElement(OverlayRenderer::DrawType::RECTANGLE, box, cv::Scalar(0, 255, 0), 2));

    // Draw detections on frame
    for (size_t i = 0; i < result.boxes.size(); i++)
    {
        cv::Rect box = result.boxes[i];
        int class_id = result.class_ids[i];
        float confidence = result.confidences[i];

        std::string label = class_names[class_id] + " " + std::to_string(confidence);
        overlay_renderer->Add(OverlayRenderer::OverlayElement(OverlayRenderer::DrawType::RECTANGLE, box, cv::Scalar(0, 255, 0), 2, true));
        overlay_renderer->Add(OverlayRenderer::OverlayElement(OverlayRenderer::DrawType::TEXT, label, cv::Point(box.x,box.y - 10), cv::Scalar(0, 255, 0), 2, true));
    }
}

/**
 * @brief Processes YOLO output and extracts detections.
 */
YOLODetector::DetectionResult YOLODetector::PostProcess(const cv::Mat& frame, const std::vector<cv::Mat>& outputs)
{
    std::vector<cv::Rect> boxes;
    std::vector<int> classIds;
    std::vector<float> confidences;

    float x_factor = static_cast<float>(frame.cols) / 640;
    float y_factor = static_cast<float>(frame.rows) / 640;

    if (outputs.empty())
    {

        return {};
    }

    cv::Mat output = outputs[0];

    if (output.empty())
    {
        std::cerr << "Output layer is empty" << std::endl;
        return {};
    }

    // Ensure correct reshaping (if the output is 3D, reshape to 2D)
    if (output.dims == 3 && output.size[0] == 1)
        output = output.reshape(1, output.size[1]);

    for (int i = 0; i < output.rows; i++)
    {
        // Extract bounding box coordinates
        float confidence = output.at<float>(i, 4);
        if (confidence > confidence_threshold)
        {
            //
            cv::Mat scores = output.row(i).colRange(5, output.cols);
            cv::Point class_id_point;
            double max_class_score;
            cv::minMaxLoc(scores, nullptr, &max_class_score, nullptr, &class_id_point);

            // Proceed if class score is above threshold
            if (max_class_score > score_threshold)
            {
                //center x, center y, width, height
                float cx = output.at<float>(i, 0);
                float cy = output.at<float>(i, 1);
                float w = output.at<float>(i, 2);
                float h = output.at<float>(i, 3);

                // Convert to pixel coordinates, multiply by factor to scale to original frame size
                int left = std::max(0, static_cast<int>((cx - 0.5f * w) * x_factor));
                int top = std::max(0, static_cast<int>((cy - 0.5f * h) * y_factor));

                // Ensure bounding box is within frame
                int width = std::min(static_cast<int>(w * x_factor), frame.cols - left);
                int height = std::min(static_cast<int>(h * y_factor), frame.rows - top);

                boxes.emplace_back(left, top, width, height);
                confidences.push_back(static_cast<float>(max_class_score));
                classIds.push_back(class_id_point.x);
            }
        }
    }

    // Apply Non-Maximum Suppression (NMS)
    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, confidences, score_threshold, nms_threshold, indices);

    DetectionResult result;

    // NMS removes the boxes with lower confidence, so we only keep the ones that are left
    for (int idx : indices)
    {
        if (idx >= boxes.size())
        {
            std::cerr << "NMS inde out of range." << std::endl; // todo: better logging system
            continue;
        }

        result.boxes.push_back(boxes[idx]);
        result.class_ids.push_back(classIds[idx]);
        result.confidences.push_back(confidences[idx]);
    }

    return result;
}

/**
 * @brief Resets the state of the detector.
 */
void YOLODetector::ResetState()
{
    call_count = 0;
    last_detection = Object::NONE;
}

/**
 * @brief Runs YOLO detection on a given frame.
 */
ObjectDetector::Object YOLODetector::Detect(const cv::Mat &frame)
{
    static int call_freq = ConfigManager::GetInstance().GetConfig<int>("object_detection_frequency");

    if (call_count++ % call_freq != 0)
        return last_detection;

    // Preprocess the frame
    cv::Mat resized = PreProcess(frame);
    cv::Mat blob = cv::dnn::blobFromImage(resized, 1 / 255.0, cv::Size(640, 640), cv::Scalar(), true, false);

    // Ensure network input is valid
    if (blob.empty())
    {
        std::cerr << "Error: Failed to create blob from image." << std::endl;
        return Object::NONE;
    }

    net.setInput(blob);
    std::vector<cv::Mat> outputs;
    net.forward(outputs, net.getUnconnectedOutLayersNames());

    // Ensure we received valid output
    if (outputs.empty())
    {
        std::cerr << "Error: YOLO did not return any valid detections." << std::endl;
        return Object::NONE;
    }

    DetectionResult result = PostProcess(frame, outputs);

    //todo: only look for person, dog, cat, car, truck
    if (this->draw_bounding_boxes)
    {
        overlay_renderer->InvalidatePersistent();
        DrawBoundingBoxes(const_cast<cv::Mat &>(frame), result);
    }

    std::string detected_object;

    if (result.class_ids.size() > 0)
        detected_object = class_names[result.class_ids[0]];

    //this->detection_count++;

    if (detected_object == "person")
        return Object::PERSON;
    else if (detected_object == "dog" || detected_object == "cat")
        return Object::PET;
    else if (detected_object == "car" || detected_object == "truck")
        return Object::CAR;
    else
        return Object::NONE;
    //todo: implement detection count
}
