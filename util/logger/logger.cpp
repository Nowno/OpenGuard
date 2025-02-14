#include "logger.hpp"
#include "../utils.hpp"
#include "../configs/user_config.hpp"
#include <iostream>
Logger::Logger()
{
    auto config_path = ConfigManager::GetInstance().GetConfig<std::string>("log_path");

    this->log_file = std::ofstream(config_path == "default" ? "logs.txt" : config_path);
}

void Logger::Log(const std::string& type, const std::string& message)
{
    std::string log_message = OpenGuard::Utils::DateTimeString() + " [" + type + "] " + message + "\n";

    std::cout << log_message;

    if (this->log_file.is_open())
    {
        std::lock_guard<std::mutex> lock(log_mutex);
        this->log_file.write(log_message.c_str(), log_message.size());
    }
}

Logger& Logger::GetInstance()
{
    static Logger instance;
    return instance;
}