#ifndef OPENGUARD_LOGGER_HPP
#define OPENGUARD_LOGGER_HPP

#include <string>
#include <fstream>
#include <mutex>

class Logger
{
    public:

    /**
     * @brief Log a message.
     * @param type The type of the message.
     * @param message The message.
     * @param save Whether to save the message to the log file.
     */
    void Log(const std::string& type, const std::string& message, bool save = true);

    /**
     * @brief Get the singleton instance of the Logger.
     * @return The Logger instance.
     */
    static Logger& GetInstance()
    {
        static Logger instance;
        return instance;
    }

    private:
    std::ofstream log_file;
    std::mutex log_mutex;   /// Mutex as we may access the log file from another thread

    /// Private constructors
    Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

};


#endif //OPENGUARD_LOGGER_HPP
