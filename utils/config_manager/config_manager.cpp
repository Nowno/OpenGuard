#include "config_manager.hpp"
#include <filesystem>
#include <iostream>
#include <fstream>

/// We use std::cerr instead of the Logger as it may not be initialized yet

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
    std::filesystem::create_directories("./config");
    std::ofstream config_file("./config/config.json");

    if (config_file.is_open())
    {
        config_file << preset_config;
        config_file.close();
    }
    else
    {
        std::cerr << "Failed to create config file." << std::endl;
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
        std::cerr << "Failed to open config file." << std::endl;
        ResetConfig();
        return;
    }

    try
    {
        config_file >> config;
    }
    catch (const nlohmann::json::exception& e)
    {
        std::cerr << "Failed to parse config file: " << e.what() << std::endl;
        ResetConfig();
    }
}

/**
 * @brief Overwrite the configuration with a new one. Requires restarting the application.
 */
void ConfigManager::OverwriteConfig(const std::string& new_config)
{
    OpenGuard::Utils::SafeCall([new_config]
    {
        nlohmann::json new_config_json = nlohmann::json::parse(new_config);

        std::ofstream config_file("./config/config.json");

        if (config_file.is_open())
        {
            config_file << std::setw(4) << new_config_json << std::endl;
            config_file.close();
        }
        else
        {
            std::cerr << "Failed to open config file for writing." << std::endl;
        }
    });
}