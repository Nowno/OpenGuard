#ifndef OPENGUARD_UTILS_HPP
#define OPENGUARD_UTILS_HPP


#include <string>
#include <fstream>
#include <vector>
#include <ctime>
#include <filesystem>

namespace OpenGuard::Utils
{
    /**
     * @brief Read a file to a string.
     * @param path The path to the file.
     * @return The contents of the file in the form of a string.
     */
    [[maybe_unused]] inline std::string FileToString(const std::string& path)
    {
        std::ifstream file(path);

        if (!file.is_open())
            throw std::runtime_error("Failed to open: " + path);

        /// Fit the whole file into a string (prone to memory issues with large files)
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        file.close();

        return content;
    }

    /**
     * @brief Read a file to a vector of strings.
     * @param path The path to the file.
     * @return The contents of the file in the from of a vector of strings.
     */
    [[maybe_unused]] inline std::vector<std::string> FileToVector(const std::string& path)
    {
        std::ifstream file(path);

        if (!file.is_open())
            throw std::runtime_error("Failed to open: " + path);

        std::vector<std::string> content;
        std::string line;

        while (std::getline(file, line))
        {
            content.push_back(line);
        }

        file.close();

        return content;
    }

    /**
     * @brief Convert a string to a boolean.
     * @param str The string to convert.
     * @return The boolean value.
     */
    inline bool StringToBool(std::string str)
    {
        for (auto& c : str)
            c = std::tolower(c);

        return str == "true";
    }

    /**
     * @brief Escape a string for shell usage.
     * @param str The string to escape.
     * @return The escaped string.
     */
    inline std::string EscapeShell(const std::string& str)
    {
        std::string escaped;

        for (char c : str)
        {
            if (c == '"')
                escaped += "\\\"";
            else
                escaped += c;
        }

        return escaped;
    }

    /**
     * @brief Get the current date and time as a string.
     * @param file_safe Whether the string should be safe for use in a file name.
     * @return The current date and time as a string.
     */
    inline std::string GetDateTimeString(bool file_safe = false)
    {
        struct tm time_info;
        time_t now = time(nullptr);

        #ifdef _WIN32
            localtime_s(&time_info, &now);
        #else
            localtime_r(&now, &timeinfo);
        #endif

        char buffer[80];

        if (file_safe)
            strftime(buffer, 80, "%Y-%m-%d %H-%M-%S", &time_info);
        else
            strftime(buffer, 80, "%Y/%m/%d %H:%M:%S", &time_info);

        return std::string(buffer);
    }

    /**
     * @brief Converts a time in a given format to unix time.
     * @param time_string The time string to convert.
     * @param format The format of the time string.
     * @return The unix time.
     */
    inline time_t TimeStringToUnix(const std::string& time_string, const std::string& format)
    {
        std::tm tm = {};
        std::istringstream ss(format);
        ss >> std::get_time(&tm, time_string.c_str());

        if (ss.fail())
            return -1;

        return std::mktime(&tm);
    }


    /**
     * @brief Execute a shell command and return the output.
     * @param command The command to execute
     * @return The output of the command.
     */
    inline std::string ExecuteCommand(const std::string& command)
    {
        /// Allocate a buffer for the output
        std::array<char, 256> buffer;


        /// Create a pipe to read the output of the command
        #ifdef _WIN32
            std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(command.c_str(), "r"), _pclose);
        #else
            std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
        #endif

        /// This really shouldn't happen, but just in case throw an error
        if (!pipe)
        {
            throw std::runtime_error("FATAL: popen() failed!");
        }

        std::string return_value;

        /// Read the output of the command and append it to the return value
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        {
            return_value += buffer.data();
        }

        return_value.erase(return_value.find_last_not_of(" \t\n\r\f\v") + 1);

        if (return_value.empty())
            return "{}";

        return return_value;
    }

    /**
     * @brief Restarts the program.
     */
    static void Restart()
    {
        std::string executable_path = std::filesystem::current_path().string() + "/OpenGuard";

        #ifdef _WIN32
            std::string command = "cmd /c \"timeout /t 1 & start \"\" \"" + executable_path + ".exe\"\"";
        #else
            std::string command = "sh -c 'sleep 1 && \"" + executable_path + "\"' &";
        #endif

        std::system(command.c_str());
        std::exit(0);
    }

    /// Try-catch block for safe execution of functions. Avoids repetition.
    template <typename T>
    inline auto SafeCall(T func)
    {
        try
        {
            return func();
        }
        catch (const std::exception& e)
        {
            printf("[XX-XX-XXXX XX:XX:XX] [ERROR] %s\n", e.what());

            /// Returns the default value for the decltype of the function
            return decltype(func())();
        }
    }


    /**
     * @brief Simple state tracker to avoid making static duplicates etc to check the previous state.
     */
    class StateTracker
    {
        public:
        StateTracker() {}

        bool GetState() const { return this->state; }

        void SetState(bool new_state) { this->state = new_state; }

        private:
        bool state = false;
    };


    /**
     * @brief Simple 2D vector, maybe unused.
     */
    struct Vec2
    {
        int x;
        int y;

        Vec2(int x, int y) : x(x), y(y) {}
        Vec2() : x(0), y(0) {}
    };

    /**
     * @brief Timer for measuring time.
     */
    struct Timer
    {
        std::chrono::time_point<std::chrono::steady_clock> start;

        Timer()
        {
            start = std::chrono::steady_clock::now();
        }

        void Reset()
        {
            start = std::chrono::steady_clock::now();
        }

        double GetDuration() const
        {
            return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        }

        bool HasElapsed(double seconds) const
        {
            return GetDuration() >= seconds;
        }
    };


    /**
     * @brief Profiler for measuring time, implemented using the previous Timer class.
     */
    struct Profiler
    {
        std::unordered_map<std::string, Timer> timers;

        void Start(const std::string& name)
        {
            timers[name].Reset();
        }

        double Stop(const std::string& name)
        {
            return timers[name].GetDuration();
        }

        void PrintResults()
        {
            for (auto& [name, timer] : timers)
            {
                printf("Measurement %s took %f seconds\n", name.c_str(), timer.GetDuration());
            }
        }
    };
}

using OpenGuard::Utils::Vec2;

#endif //OPENGUARD_UTILS_HPP