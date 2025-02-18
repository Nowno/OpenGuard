#include "logger.hpp"
#include "../utils.hpp"
#include "../configs/user_config.hpp"
#include <iostream>
Logger::Logger()
{
    //auto config_path = ConfigManager::GetInstance().GetConfig<std::string>("log_path"); todo readd this
    this->log_file = std::ofstream("logs.txt", std::ios::app);

    if (!this->log_file.is_open())
    {
        std::cerr << "Error: Could not open log file." << std::endl;
    }
}

void Logger::Log(const std::string& type, const std::string& message, bool save)
{
    std::string log_message = "[" + OpenGuard::Utils::DateTimeString(false) + "] [" + type + "] " + message + "\n";
    std::cout << log_message;

    if (!save) return;

    if (this->log_file.is_open())
    {
        std::lock_guard<std::mutex> lock(log_mutex);
        this->log_file.write(log_message.c_str(), log_message.size());
        this->log_file.flush();
    }
    else
    {
        std::cerr << "Error: Could not write to log file." << std::endl;
    }
}
