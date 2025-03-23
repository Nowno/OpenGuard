#include <iostream>
#include <sstream>

#include "logger.hpp"
#include "../config_manager/config_manager.hpp"
#include "../../core/hook_manager/hook_manager.hpp"
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
void Logger::Log(const std::string_view &type, const std::string_view &message, bool save)
{
    /// Form the log message and print it.
    std::ostringstream oss;
    oss << "[" << OpenGuard::Utils::GetDateTimeString(false) << "] [" << type << "] " << message << "\n";
    std::string log_message = oss.str();

    std::cout << log_message;
    /// If there was a fatal error, execute the appropriate hooks as the user may want to handle it
    if (type == "FATAL")
        HookManager::GetInstance().ExecuteHooks("on_fatal", {{"message", std::string(message)}});


    /// If we don't want to save the message, no need to proceed.
    if (!save)
        return;

    if (this->log_file.is_open())
    {
        std::lock_guard<std::mutex> lock(log_mutex);
        this->log_file.write(log_message.c_str(), log_message.size());
        this->log_file.flush(); /// Flush to finalize the write

        /// If the buffer is full, pop the first element.
        if (log_buffer.size() >= MAX_LOGS)
            log_buffer.pop_front();

        /// Push the new log message to the buffer.
        log_buffer.push_back(log_message);

        HookManager::GetInstance().ExecuteHooks("on_log", {{"message", log_message}});
    }
    else
    {
        std::cerr << "[XX-XX-XXXX XX:XX:XX] [ERROR] Could not write to log file: " << message << std::endl;
    }
}

/// Dump the log buffer to a string.
std::string Logger::DumpBuffer()
{
    std::lock_guard<std::mutex> lock(log_mutex);
    std::string buffer;

    /// Concatenate all the logs in the buffer.
    for (const auto& log : log_buffer)
    {
        buffer += log + "\n";
    }

    /// And return it, used in the panel.
    return buffer;
}