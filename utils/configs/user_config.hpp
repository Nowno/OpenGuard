#ifndef OPENGUARD_USER_CONFIG_HPP
#define OPENGUARD_USER_CONFIG_HPP

#include <string>
#include <unordered_map>
#include <unordered_set>
#include "../libs/json.hpp"
#include "../logger/logger.hpp"
#include "../utils.hpp"

class ConfigManager
{
    public:
    /**
     * @brief Get the singleton instance of the ConfigManager.
     * @return The ConfigManager instance.
     */
    static ConfigManager& GetInstance()
    {
        static ConfigManager instance;
        return instance;
    }

    /**
     * @brief Get a configuration value.
     * @tparam T The type of the value.
     * @param key The key of the value.
     * @return The value.
     */
    template<typename T> T GetConfig(const std::string& key) const
    {
        try
        {
            if constexpr (std::is_same_v<T, bool>)
            {
                /// Handle "true" and "false" strings
                return OpenGuard::Utils::StringToBool(this->config[key].get<std::string>());
            }
            else if constexpr (std::is_same_v<T, std::unordered_set<std::string>>)
            {
                /// Edge case for unordered set
                std::unordered_set<std::string> set;
                for (const auto& item : this->config[key])
                {
                    set.insert(item.get<std::string>());
                }
                return set;
            }
            else
            {
                /// Default case, return the value as whatever type the user wants
                return config.at(key).get<T>();
            }
        }
        catch (nlohmann::json::exception &e)
        {
            Logger::GetInstance().Log("ERROR", "Failed to get config value: " + key + " " + e.what());

            return T(); /// If an error occurs, return the default value of the type (0, false, etc)
        }
    }

    private:
    /// Private constructor for singleton pattern
    ConfigManager();
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    /// Reset the configuration to the default preset
    void ResetConfig();

    /// Load the configuration from the file
    void LoadConfig();

    /// JSON configuration object
    nlohmann::json config;

    /// Default preset configuration to write to the file if it doesn't exist
    const std::string preset_config = R"({
        "model_path": "./core/detection/yolo/models/yolov5s.onnx",
        "classes_path": "./core/detection/yolo/models/coco.names",
        "output_path": "./output",
        "ffmpeg_path": "ffmpeg",
        "frame_width": 640,
        "frame_height": 480,
        "frame_rate": 15,
        "motion_threshold": 0.1,
        "confidence_threshold": 0.35,
        "score_threshold": 0.3,
        "nms_threshold": 0.45,
        "use_gpu": true,
        "pre_record_length": 2,
        "post_record_length": 2,
        "python_prefix": "python",
        "log_path": "default",
        "record_worthy": ["person", "car", "pet"],
        "object_detection_frequency": 3
    })";
};


#endif //OPENGUARD_USER_CONFIG_HPP