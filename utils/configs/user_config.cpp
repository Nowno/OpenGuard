#include "user_config.hpp"
#include <filesystem>
#include <iostream>
#include <fstream>

/**
 * @brief Initiate the config manager.
 */
ConfigManager::ConfigManager()
{
    std::string config_path = "./config/config.json";

    if (!std::filesystem::exists(config_path))
    {
        ResetConfig();
    }

    LoadConfig();
}

/**
 * @brief Reset the configuration to the default preset.
 */
void ConfigManager::ResetConfig()
{
    Logger::GetInstance().Log("INFO", "Resetting config to default.");

    std::filesystem::create_directories("./config");
    std::ofstream config_file("./config/config.json");

    if (config_file.is_open())
    {
        config_file << preset_config;
        config_file.close();
    }
    else
    {
        Logger::GetInstance().Log("ERROR", "Failed to create default config file.");
    }
}

/**
 * @brief Load the configuration from the file.
 */
void ConfigManager::LoadConfig()
{
    std::ifstream config_file("./config/config.json");

    if (!config_file.is_open())
    {
        Logger::GetInstance().Log("ERROR", "Failed to open config file, resetting to default.");
        ResetConfig();
        return;
    }

    try
    {
        config_file >> config;
    }
    catch (const nlohmann::json::exception& e)
    {
        Logger::GetInstance().Log("ERROR", "Error parsing config file: " + std::string(e.what()));
        ResetConfig();
    }
}
