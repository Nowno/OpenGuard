#ifndef OPENGUARD_USER_CONFIG_HPP
#define OPENGUARD_USER_CONFIG_HPP

#include <string>
#include <unordered_map>

//Hard coded for now

std::unordered_map<std::string, std::string> user_config =
{
    {"model_path", "C:\\Users\\Nono\\Desktop\\Stuff\\School\\FP - Surveilance\\OpenGuard\\core\\detection\\yolo\\models\\yolov5s.onnx"}, // Relative didn't work with OpenCV
    {"classes_path", "C:\\Users\\Nono\\Desktop\\Stuff\\School\\FP - Surveilance\\OpenGuard\\core\\detection\\yolo\\models\\coco.names"},
    {"confidence_threshold", "0.4"},
    {"score_threshold", "0.2"},
    {"nms_threshold", "0.4"},
    {"use_gpu", "0"}
};


#endif //OPENGUARD_USER_CONFIG_HPP
