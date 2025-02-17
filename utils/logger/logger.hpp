#ifndef OPENGUARD_LOGGER_HPP
#define OPENGUARD_LOGGER_HPP

#include <string>
#include <fstream>
#include <mutex>

class Logger
{
    public:

    void Log(const std::string& type, const std::string& message);
    static Logger& GetInstance()
    {
        static Logger instance;
        return instance;
    }

    private:
    std::ofstream log_file;
    std::mutex log_mutex;

    Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

};


#endif //OPENGUARD_LOGGER_HPP
