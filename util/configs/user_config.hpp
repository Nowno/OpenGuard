#ifndef OPENGUARD_USER_CONFIG_HPP
#define OPENGUARD_USER_CONFIG_HPP

#include <string>
#include <unordered_map>
#include "../libs/json.hpp"

class ConfigManager
{
    public:
    static ConfigManager& GetInstance();
    template<typename T> T GetConfig(const std::string& key) const
    {
        try
        {
            return this->config[key].get<T>();
        }
        catch (nlohmann::json::exception &e)
        {
            //Todo: log
            return T();
        }

    }

    private:

    ConfigManager();
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    void ResetConfig();
    void LoadConfig();

    nlohmann::json config;

    std::string preset_config = R"({
        "model_path": "./core/detection/yolo/models/yolov5s.onnx",
        "classes_path": "./core/detection/yolo/models/coco.names",
        "output_path": "./output",
        "ffmpeg_path": "ffmpeg",
        "frame_width": "640",
        "frame_height": "480",
        "frame_rate": "15",
        "motion_threshold": "0.1",
        "confidence_threshold": "0.35",
        "score_threshold": "0.3",
        "nms_threshold": "0.45",
        "use_gpu": "true",
        "pre_record_length": "2",
        "post_record_length": "2",
        "python_prefix": "python"
    })";
};


#endif //OPENGUARD_USER_CONFIG_HPP