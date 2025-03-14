#ifndef OPENGUARD_COMMAND_PROCESSOR_HPP
#define OPENGUARD_COMMAND_PROCESSOR_HPP

#include <string>
#include <json/json.hpp>
#include <unordered_map>

class CommandProcessor
{
    public:
    static CommandProcessor& CommandProcessor::GetInstance()
    {
        static CommandProcessor instance;
        return instance;
    }

    void Process(const std::string& command);

    private:
    CommandProcessor();
    CommandProcessor(const CommandProcessor&) = delete;
    CommandProcessor& operator=(const CommandProcessor&) = delete;

    nlohmann::json ParseCommand(const std::string& command);

    void InvokeCommand(const std::string& command_type, const std::string& command_args);

    std::unordered_map<std::string, void (CommandProcessor::*)(const std::string&)> command_map;
};


#endif //OPENGUARD_COMMAND_PROCESSOR_HPP
