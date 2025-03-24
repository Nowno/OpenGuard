#ifndef OPENGUARD_CONFIG_MANAGER_HPP
#define OPENGUARD_CONFIG_MANAGER_HPP

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <json/json.hpp>
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

    /**
     * @brief Return the full configuration.
     * @return The full configuration.
     */
    nlohmann::json GetFullConfig() const { return config; }

    /**
     * @brief Overwrite the configuration with a new one. Requires restarting the application.
     * @param new_config The new configuration.
     */
    void OverwriteConfig(const std::string& new_config);

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
    const std::string preset_config = "{\n""    \"ffmpeg_path\": \"C:\\\\Users\\\\Nono\\\\Desktop\\\\Stuff\\\\Coding\\\\Libs\\\\ffmpeg-master-latest-win64-gpl\\\\bin\\\\ffmpeg.exe\",\n"
    "    \"frame_height\": 480,\n"
    "    \"frame_rate\": 15,\n"
    "    \"frame_width\": 640,\n"
    "    \"log_path\": \"./config\",\n"
    "    \"mog2_detect_shadows\": \"true\",\n"
    "    \"mog2_history\": 300,\n"
    "    \"mog2_motion_threshold\": 0.2,\n"
    "    \"mog2_sensitivity\": 10,\n"
    "    \"mog2_draw_bb\": \"false\",\n"
    "    \"output_path\": \"./output\",\n"
    "    \"post_record_length\": 5,\n"
    "    \"pre_record_length\": 5,\n"
    "    \"python_prefix\": \"py\",\n"
    "    \"server_password\": \"nimda\",\n"
    "    \"server_port\": 9002,\n"
    "    \"server_username\": \"admin\",\n"
    "    \"yolo_classes_path\": \"C:\\\\Users\\\\Nono\\\\Desktop\\\\Stuff\\\\School\\\\FP - Surveilance\\\\OpenGuard\\\\core\\\\detection\\\\yolo\\\\models\\\\coco.names\",\n"
    "    \"yolo_confidence_threshold\": 0.35,\n"
    "    \"yolo_model_path\": \"C:\\\\Users\\\\Nono\\\\Desktop\\\\Stuff\\\\School\\\\FP - Surveilance\\\\OpenGuard\\\\core\\\\detection\\\\yolo\\\\models\\\\yolov5n.onnx\",\n"
    "    \"yolo_object_detection_frequency\": 3,\n"
    "    \"yolo_record_worthy\": [\n"
    "        \"person\",\n"
    "        \"car\",\n"
    "        \"pet\"\n"
    "    ],\n"
    "    \"yolo_resolution\": 640,\n"
    "    \"yolo_score_threshold\": 0.3,\n"
    "    \"yolo_use_gpu\": \"false\",\n"
    "    \"yolo_draw_bb\": \"true\",\n"
    "    \"render_hud\": \"true\",\n"
    "    \"render_debug\": \"true\"\n"
    "}";
};


#endif //OPENGUARD_CONFIG_MANAGER_HPP