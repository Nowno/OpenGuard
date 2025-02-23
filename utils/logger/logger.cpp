#include "logger.hpp"
#include "../configs/user_config.hpp"
#include "../../core/hook_manager/hook_manager.hpp"
#include <iostream>

/**
 * @brief Constructor: Initializes the logger.
 */
Logger::Logger()
{
    auto log_path = ConfigManager::GetInstance().GetConfig<std::string>("log_path");

    log_path = log_path.empty() || log_path == "default" ? "logs.txt" : log_path;

    this->log_file = std::ofstream(log_path, std::ios::app);

    if (!this->log_file.is_open())
    {
        std::cerr << "Error: Could not open log file." << std::endl;
    }
}

/**
 * @brief Log a message.
 */
void Logger::Log(const std::string& type, const std::string& message, bool save)
{
    std::string log_message = "[" + OpenGuard::Utils::GetDateTimeString(false) + "] [" + type + "] " + message + "\n";
    std::cout << log_message; /// Form the log message and print it.

    if (type == "FATAL")      /// If there was a fatal error, execute the appropriate hooks as the user may want to handle it
        HookManager::GetInstance().ExecuteHooks("on_fatal", {{"message", message}});

    if (!save)                /// If we don't want to save the message, no need to proceed.
        return;

    if (this->log_file.is_open())
    {
        std::lock_guard<std::mutex> lock(log_mutex);
        this->log_file.write(log_message.c_str(), log_message.size());
        this->log_file.flush(); /// Flush to finalize the write
    }
    else
    {
        Logger::Log("ERROR", "Failed to write to log file.", false);
    }
}
