#include "command_processor.hpp"
#include <json/json.hpp>
#include "../../utils/logger/logger.hpp"
#include "../../utils/config_manager/config_manager.hpp"
#include "../hook_manager/hook_manager.hpp"
using json = nlohmann::json;

CommandProcessor::CommandProcessor()
{
    command_map =
    {
        {"auth", &CommandProcessor::Login},
        {"pause", &CommandProcessor::Pause}
    };

}

std::string CommandProcessor::Process(const std::string &command)
{
    json parsed_command = ParseCommand(command);

    if (parsed_command.empty())
        return "";

    std::string command_type = parsed_command["type"].get<std::string>();
    std::string command_args = parsed_command["args"].dump();

    return InvokeCommand(command_type, command_args);
}

json CommandProcessor::ParseCommand(const std::string &command)
{
    json command_json = OpenGuard::Utils::SafeCall([&](){ return json::parse(command);});
    if (command_json.empty() ||command_json.is_null() || !command_json.contains("type") || !command_json.contains("args"))
    {
        Logger::GetInstance().Log("ERROR", "Invalid json received from client: " + command, false);
        return json();
    }

    return command_json;
}

std::string CommandProcessor::InvokeCommand(const std::string &command_type, const std::string &command_args)
{
    if (command_map.find(command_type) == command_map.end())
    {
        Logger::GetInstance().Log("ERROR", "Invalid command type: " + command_type, false);
        return "";
    }


    return (this->*command_map[command_type])(command_args);
}

nlohmann::json CommandProcessor::ValidateArgs(const std::string &args, const std::vector<std::string>& expected_fields)
{
    json args_json = OpenGuard::Utils::SafeCall([&](){ return json::parse(args);});

    if (args_json.empty() || args_json.is_null())
    {
        Logger::GetInstance().Log("ERROR", "Invalid json received from client: " + args, false);
        return json();
    }

    for (const auto& field : expected_fields)
    {
        if (!args_json.contains(field))
        {
            Logger::GetInstance().Log("ERROR", "Missing field in command args: " + field, false);
            return json();
        }
    }

    return args_json;
}


std::string CommandProcessor::Login(const std::string &args)
{
    json args_json = ValidateArgs(args, {"username", "password"});

    if (!args_json.empty() && args_json["username"] == ConfigManager::GetInstance().GetConfig<std::string>("server_username") &&
                              args_json["password"] == ConfigManager::GetInstance().GetConfig<std::string>("server_password"))
    {
        return "authenticated";
    }

    return "failed";
}

std::string CommandProcessor::Pause(const std::string &args)
{
    json args_json = ValidateArgs(args, {"duration"});

    if (args_json.empty())
        return "failed";

    std::string duration = args_json["duration"].get<std::string>();

    HookManager::GetInstance().AppendOutput("on_motion", "{\"pause_system\": " + duration + "}");

    return "paused";
}