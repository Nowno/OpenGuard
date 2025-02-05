#ifndef OPENGUARD_UTILS_HPP
#define OPENGUARD_UTILS_HPP

#endif //OPENGUARD_UTILS_HPP
#include <string>
#include <fstream>
#include <vector>
namespace OpenGuard::Utils
{
    //Open file and return contents
    inline std::string FileToString(const std::string& path)
    {
        std::ifstream file(path);

        if (!file.is_open())
            throw std::runtime_error("Failed to open: " + path);

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        file.close();

        return content;
    }

    inline std::vector<std::string> FileToVector(const std::string& path)
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

    struct Vec2
    {
        int x;
        int y;

        Vec2(int x, int y) : x(x), y(y) {}
        Vec2() : x(0), y(0) {}
    };

    struct Timer
    {
        std::chrono::time_point<std::chrono::steady_clock> start; // Use steady_clock

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

    //todo: pretty print and error logging

}

using OpenGuard::Utils::Vec2;