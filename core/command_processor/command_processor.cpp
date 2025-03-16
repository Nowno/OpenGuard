#include "command_processor.hpp"
#include <json/json.hpp>
#include "../../utils/logger/logger.hpp"
#include "../../utils/config_manager/config_manager.hpp"

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
    json command_json = OpenGuard::Utils::SafeCall([&](){ return json::parse(command);});

    if (command_json.empty() ||command_json.is_null() || !command_json.contains("type") || !command_json.contains("args"))
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

    json args_json =  OpenGuard::Utils::SafeCall([&](){ return json::parse(args);});

    if (args_json.is_null())
    {
        Logger::GetInstance().Log("ERROR", "Failed to parse login args.");
        return "failed";
    }

    if (!args_json.contains("username") || !args_json.contains("password"))
    {
        Logger::GetInstance().Log("ERROR", "Login args missing username or password.");
        return "failed";
    }

    if (args_json["username"] == ConfigManager::GetInstance().GetConfig<std::string>("username") &&
        args_json["password"] == ConfigManager::GetInstance().GetConfig<std::string>("password"))
    {
        return "authenticated";
    }

    return "failed";
}