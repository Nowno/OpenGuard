#include <json/json.hpp>
#include <opencv2/opencv.hpp>

#include "command_processor.hpp"
#include "../../utils/logger/logger.hpp"
#include "../../utils/config_manager/config_manager.hpp"
#include "../../utils/utils.hpp"
#include "../server/ws_server.hpp"
using json = nlohmann::json;

CommandProcessor::CommandProcessor()
{
    command_map =
    {
        {"auth", &CommandProcessor::Login},
        {"pause_system", &CommandProcessor::PauseSystem},
        {"snapshot", &CommandProcessor::Snapshot},
        {"restart", &CommandProcessor::Restart},
        {"set_config", &CommandProcessor::Restart},
        {"get_logs", &CommandProcessor::GetLogs}
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

void CommandProcessor::SetStreaming(const std::string& stream, bool streaming)
{
    if (stream == "snapshot")
    {
        this->snapshot_streaming = streaming;
    }
    else if (stream == "log")
    {
        this->log_streaming = streaming;
    }
}

bool CommandProcessor::GetStreaming(const std::string& stream)
{
    return stream == "snapshot" ? this->snapshot_streaming : this->log_streaming;
}

HookManager::HookHandle CommandProcessor::GetHookID(CommandProcessor::Hooks hook)
{
    if (hook == CommandProcessor::Hooks::SNAPSHOT)
    {
        return this->snapshot_hook_id;
    }
    else if (hook == CommandProcessor::Hooks::LOG)
    {
        return this->log_hook_id;
    }

    return 0;
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

std::string CommandProcessor::PauseSystem(const std::string &args)
{
    json args_json = ValidateArgs(args, {"duration"});

    if (args_json.empty())
        return "failed";

    args_json = args_json["duration"];
    std::string duration = args_json.is_string() ? args_json.get<std::string>() : std::to_string(args_json.get<int>());

    //HookManager::GetInstance().AppendOutput("on_motion", "{\"pause_system\": " + duration + "}");

    return "success";
}

std::string CommandProcessor::Snapshot(const std::string &args)
{
    json args_json = ValidateArgs(args, {"status"});

    if (args_json.empty())
        return "failed";

    std::string status = args_json["status"].get<std::string>();
    bool screenshot = status == "screenshot";

    if (status == "stop" && snapshot_streaming)
    {
        HookManager::GetInstance().UnregisterHook("on_render", snapshot_hook_id);
        snapshot_streaming = false;
        return "success";
    }

    auto snapshot_hook = HookManager::Hook(HookManager::HookType::NATIVE, [screenshot](const std::unordered_map<std::string, std::string>& args) -> std::string
    {
        json response = {{"type", (screenshot ? "screenshot" : "snapshot_stream")}, {"image", args.at("frame")}};

        WSServer::GetInstance().Send(response.dump());

        if (!CommandProcessor::GetInstance().GetStreaming("snapshot"))
        {
            auto hook_id = CommandProcessor::GetInstance().GetHookID(Hooks::SNAPSHOT);
            HookManager::GetInstance().UnregisterHook("on_render", hook_id);
        }

        return "";
    }, true, 0, screenshot);

    if (!snapshot_streaming)
    {
        this->snapshot_hook_id = HookManager::GetInstance().RegisterHook("on_render", snapshot_hook);
        snapshot_streaming = true;
    }

    return "success";
}

std::string CommandProcessor::GetLogs(const std::string &args)
{
    json args_json = ValidateArgs(args, {"type"});

    if (args_json.empty())
        return "failed";

    auto log_type = args_json["type"].get<std::string>();

    if (log_type == "dump")
    {
        json response = {{"type", "log_dump"}, {"message", Logger::GetInstance().DumpBuffer()}};
        WSServer::GetInstance().Send(response.dump());
    }
    else if (log_type == "stream_start")
    {
        auto log_hook = HookManager::Hook(HookManager::HookType::NATIVE, [](const std::unordered_map<std::string, std::string>& args) -> std::string
        {
            json response = {{"type", "log"}, {"message", args.at("message")}};
            WSServer::GetInstance().Send(response.dump());

            if (!CommandProcessor::GetInstance().GetStreaming("log"))
            {
                auto hook_id = CommandProcessor::GetInstance().GetHookID(Hooks::LOG);
                HookManager::GetInstance().UnregisterHook("on_log", hook_id);
            }

            return "";
        }, true, 0, false);

        if (!CommandProcessor::GetInstance().GetStreaming("log"))
        {
            this->log_hook_id = HookManager::GetInstance().RegisterHook("on_log", log_hook);
            log_streaming = true;
        }
    }
    else if (log_type == "stream_stop")
    {
        HookManager::GetInstance().UnregisterHook("on_log", log_hook_id);
        log_streaming = false;
    }

    return "success";
}

std::string CommandProcessor::Restart(const std::string &args)
{

    WSServer::GetInstance().CloseServer();
    OpenGuard::Utils::Restart();

    /// This should never be reached
    return "";
}