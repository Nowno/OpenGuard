//
// Created by Nono on 07/02/2025.
//
#include "user_config.hpp"
#include <filesystem>
#include <iostream>
#include <fstream>

ConfigManager::ConfigManager()
{
    //todo maybe refactor a bit here
    LoadConfig();
}

void ConfigManager::ResetConfig()
{
    std::string config_path = "./config/config.json";
    std::filesystem::create_directories("./config");
    std::ofstream config_file(config_path);

    config_file << this->preset_config;
    config_file.close();
}

void ConfigManager::LoadConfig()
{
    std::string config_path = "./config/config.json";

    if (!std::filesystem::exists(config_path))
    {
        ResetConfig();
        return;
    }

    std::ifstream config_file(config_path);
    try
    {
        config_file >> this->config;
    }
    catch (nlohmann::json::exception& e)
    {
        ResetConfig();
        this->config = nlohmann::json::parse(this->preset_config);
    }
}


ConfigManager& ConfigManager::GetInstance()
{
    static ConfigManager instance;
    return instance;
}
