#ifndef OPENGUARD_COMMAND_PROCESSOR_HPP
#define OPENGUARD_COMMAND_PROCESSOR_HPP

#include <string>
#include <json/json.hpp>
#include <unordered_map>

class CommandProcessor
{
    public:
    static CommandProcessor& GetInstance()
    {
        static CommandProcessor instance;
        return instance;
    }

    std::string Process(const std::string& command);

    private:
    CommandProcessor();
    CommandProcessor(const CommandProcessor&) = delete;
    CommandProcessor& operator=(const CommandProcessor&) = delete;

    nlohmann::json ParseCommand(const std::string& command);
    nlohmann::json ValidateArgs(const std::string &args, const std::vector<std::string>& expected_fields);

    std::string InvokeCommand(const std::string& command_type, const std::string& command_args);

    ////////////////////// Commands //////////////////////
    std::string Login(const std::string& args);
    std::string Pause(const std::string& args);
    std::string Restart(const std::string& args);
    std::string Snapshot(const std::string& args);

    std::unordered_map<std::string, std::string (CommandProcessor::*)(const std::string&)> command_map;
};
#endif //OPENGUARD_COMMAND_PROCESSOR_HPP
