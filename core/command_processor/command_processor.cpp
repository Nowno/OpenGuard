#include "command_processor.hpp"
#include <json/json.hpp>
#include "../../utils/logger/logger.hpp"

using json = nlohmann::json;

CommandProcessor::CommandProcessor()
{
    command_map =
    {
        {"login", &CommandProcessor::Login}
    };

}

//define command map


std::string CommandProcessor::Process(const std::string &command)
{
    json parsed_command = ParseCommand(command);

    if (parsed_command.empty())
        return "";

    std::string command_type = parsed_command["type"];
    std::string command_args = parsed_command["args"];

    return InvokeCommand(command_type, command_args);
}

json CommandProcessor::ParseCommand(const std::string &command)
{
    json command_json;

    try
    {
        command_json = json::parse(command);
    }
    catch (const std::exception &e)
    {
        command_json = "{}";
    }

    if (command_json.empty() || !command_json.contains("type") || !command_json.contains("args"))
    {
        Logger::GetInstance().Log("ERROR", "Invalid json received from client: " + command);
    }

    return command_json;
}

std::string CommandProcessor::InvokeCommand(const std::string &command_type, const std::string &command_args)
{
    if (command_map.find(command_type) == command_map.end())
    {
        Logger::GetInstance().Log("ERROR", "Invalid command type: " + command_type);
        return "";
    }

    return (this->*command_map[command_type])(command_args);
}

std::string CommandProcessor::Login(const std::string &args)
{
    Logger::GetInstance().Log("INFO", "Login command received.");

    printf("%s\n", args.c_str());

    return "Login command received.";
}