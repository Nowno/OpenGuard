#include <json/json.hpp>
#include <b64/base64.hpp>

#include "command_processor.hpp"
#include "../../utils/logger/logger.hpp"
#include "../../utils/config_manager/config_manager.hpp"
#include "../../utils/utils.hpp"
#include "../server/ws_server.hpp"
using json = nlohmann::json;

CommandProcessor::CommandProcessor()
{
    /// Map the command type to the corresponding function
    command_map =
    {
        {"auth", &CommandProcessor::Login},
        {"pause_system", &CommandProcessor::PauseSystem},
        {"snapshot", &CommandProcessor::Snapshot},
        {"restart", &CommandProcessor::Restart},
        {"set_config", &CommandProcessor::SetConfig},
        {"get_logs", &CommandProcessor::GetLogs},
        {"get_videos", &CommandProcessor::GetVideos},
        {"get_hooks", &CommandProcessor::GetHooks}
    };

}

std::string CommandProcessor::Process(const std::string &command)
{
    /// Parse the command and check if it is valid
    json parsed_command = ParseCommand(command);

    if (parsed_command.empty())
        return "";

    /// Extract the command type and arguments
    std::string command_type = parsed_command["type"].get<std::string>();
    std::string command_args = parsed_command["args"].dump();

    /// Invoke the appropriate command
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

    auto status = args_json["status"].get<std::string>();
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

std::string CommandProcessor::SetConfig(const std::string &args)
{
    json args_json = ValidateArgs(args, {"config"});

    if (args_json.empty())
        return "failed";

    ConfigManager::GetInstance().OverwriteConfig(args_json["config"]);

    return "success";
}

std::string CommandProcessor::GetVideos(const std::string &args)
{
    json args_json = ValidateArgs(args, {"type"});

    if (args_json.empty())
        return "failed";

    auto video_type = args_json["type"].get<std::string>();

    if (video_type == "list")
    {
        auto video_dir = ConfigManager::GetInstance().GetConfig<std::string>("output_path");
        std::vector<std::string> videos;

        for (const auto& entry : std::filesystem::directory_iterator(video_dir))
        {
            videos.push_back(entry.path().filename().string());
        }

        json response = {{"type", "video_list"}, {"videos", videos}};
        WSServer::GetInstance().Send(response.dump());
    }
    else if (video_type == "stream")
    {
        args_json = ValidateArgs(args, {"video"});

        if (args_json.empty())
            return "failed";

        auto video = args_json["video"].get<std::string>();
        auto video_dir = ConfigManager::GetInstance().GetConfig<std::string>("output_path");

        if (std::filesystem::exists(video_dir + "/" + video))
        {
            std::thread([video, video_dir]()
            {
                std::ifstream video_file(video_dir + "/" + video, std::ios::binary);

                if (!video_file.is_open())
                    return;

                Logger::GetInstance().Log("INFO", "Streaming video: " + video);

                const size_t buffer_size = 128 * 1024;
                std::vector<char> buffer(buffer_size);

                while (video_file.read(buffer.data(), buffer.size()) || video_file.gcount())
                {
                    auto bytes_read = video_file.gcount();
                    if (bytes_read > 0)
                    {
                        std::string chunk = std::string(buffer.data(), bytes_read);
                        std::string base64_chunk = base64::to_base64(chunk);

                        json response = {{"type", "video_stream"}, {"args", {{"video", video}, {"data", base64_chunk}}}};
                        WSServer::GetInstance().Send(response.dump());

                        /// Avoid flooding the client
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                }
                video_file.close();
                json response = {{"type", "video_stream"}, {"args", {{"video", video}, {"data", "stop"}}}};
                WSServer::GetInstance().Send(response.dump());
                Logger::GetInstance().Log("INFO", "Finished streaming video: " + video);
            }).detach();
        }
        else
        {
            return "failed";
        }
    }
    else if (video_type == "delete")
    {
        args_json = ValidateArgs(args, {"video"});

        if (args_json.empty())
            return "failed";

        auto video = args_json["video"].get<std::string>();
        auto video_dir = ConfigManager::GetInstance().GetConfig<std::string>("output_path");

        if (std::filesystem::exists(video_dir + "/" + video))
        {
            std::filesystem::remove(video_dir + "/" + video);
        }
    }

    return "success";
}

std::string CommandProcessor::GetHooks(const std::string &args)
{
    json args_json = ValidateArgs(args, {"type"});

    if (args_json.empty())
        return "failed";

    auto request_type = args_json["type"].get<std::string>();

    if (request_type == "list")
    {
        auto events = HookManager::GetInstance().GetEvents();
        std::vector<std::string> hook_files;
        for (const auto& event : events)
        {
            auto hooks = HookManager::GetInstance().GetHooks(event);
            for (const auto& hook : hooks)
            {
                if (hook.type == HookManager::HookType::EXTERNAL)
                {
                    hook_files.push_back(hook.script_path);
                }
            }
        }

        json response = {{"type", "get_hooks"}, {"args", {{"type", "list"}, {"hooks", hook_files}}}};
        WSServer::GetInstance().Send(response.dump());
    }
    else if (request_type == "delete")
    {
        args_json = ValidateArgs(args, {"hook"});

        if (args_json.empty())
            return "failed";

        /// Unregister -> Delete
        auto hook_name = args_json["hook"].get<std::string>();

        /// Get the event name
        auto event = std::filesystem::path(hook_name).parent_path().filename().string();

        /// Get all hooks for the event and find the one with the matching path
        auto hooks = HookManager::GetInstance().GetHooks(event);

        for (const auto& _hook : hooks)
        {
            if (_hook.script_path == "./hooks/" + hook_name)
            {
                HookManager::GetInstance().UnregisterHook(event, _hook.id);
                break;
            }
        }

        std::filesystem::remove("./hooks/" + hook_name);
    }
    else if (request_type == "add")
    {
        args_json = ValidateArgs(args, {"event", "file_name", "content"});

        if (args_json.empty())
            return "failed";

        auto event = args_json["event"].get<std::string>();
        auto file_name = args_json["file_name"].get<std::string>();
        auto content = args_json["content"].get<std::string>();

        /// Get the hook path
        auto hook_path = "./hooks/" + event + "/" + file_name;

        /// Write the content to the file
        std::ofstream hook_file(hook_path);
        hook_file << content;
        hook_file.close();

        /// Register the hook
        HookManager::GetInstance().RegisterHook(event, HookManager::Hook(HookManager::HookType::EXTERNAL, hook_path));
    }
    else if (request_type == "save")
    {
        args_json = ValidateArgs(args, {"file_name", "content"});

        if (args_json.empty())
            return "failed";

        auto file_name = args_json["file_name"].get<std::string>();
        auto event = std::filesystem::path(file_name).parent_path().filename().string();
        auto content = args_json["content"].get<std::string>();

        /// Get the hook path
        auto hook_path = "./hooks/" + file_name;
        /// If the direcrory doesn't exist, create it
        if (!std::filesystem::exists("./hooks/" + event))
        {
            std::filesystem::create_directories("./hooks/" + event);
        }

        /// Write the content to the file
        std::ofstream hook_file(hook_path);
        hook_file << content;
        hook_file.close();

        /// If the event doesn't have the hook registered, register it
        auto hooks = HookManager::GetInstance().GetHooks(event);
        bool found = false;
        for (const auto& hook : hooks)
        {
            if (hook.script_path == hook_path)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            HookManager::GetInstance().RegisterHook(event, HookManager::Hook(HookManager::HookType::EXTERNAL, hook_path));
        }
    }
    else if (request_type == "get")
    {
        args_json = ValidateArgs(args, {"hook"});

        if (args_json.empty())
            return "failed";

        auto hook = args_json["hook"].get<std::string>();

        std::ifstream hook_file("./hooks/" + hook);
        std::string content((std::istreambuf_iterator<char>(hook_file)), std::istreambuf_iterator<char>());
        hook_file.close();



        json response = {{"type", "get_hooks"}, {"args", {{"type", "content"}, {"content", content}, {"hook", hook}}}};
        WSServer::GetInstance().Send(response.dump());
    }

    return "success";
}