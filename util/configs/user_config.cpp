//
// Created by Nono on 07/02/2025.
//
#include "user_config.hpp"


namespace OpenGuard
{
    std::unordered_map<std::string, std::string> user_config =
            {
                    {"model_path", "C:\\Users\\Nono\\Desktop\\Stuff\\School\\FP - Surveilance\\OpenGuard\\core\\detection\\yolo\\models\\yolov5s.onnx"},
                    {"classes_path", "C:\\Users\\Nono\\Desktop\\Stuff\\School\\FP - Surveilance\\OpenGuard\\core\\detection\\yolo\\models\\coco.names"},
                    {"output_path", "C:\\Users\\Nono\\Desktop\\Stuff\\School\\FP - Surveilance\\OpenGuard\\output"},
                    {"ffmpeg_path", "C:\\Users\\Nono\\Desktop\\Stuff\\Coding\\Libs\\ffmpeg-master-latest-win64-gpl\\bin\\ffmpeg.exe"},
                    {"frame_width", "640"},
                    {"frame_height", "480"},
                    {"frame_rate", "15"},
                    {"motion_threshold", "0.1"},
                    {"confidence_threshold", "0.4"},
                    {"score_threshold", "0.2"},
                    {"nms_threshold", "0.4"},
                    {"use_gpu", "true"},
                    {"pre_record_length", "2"},
                    {"post_record_length", "2"}
                    {"python_prefix", "py"}
            };
}