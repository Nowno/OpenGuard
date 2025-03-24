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
        {"get_hooks", &CommandProcessor::GetHooks},
        {"roi_select", &CommandProcessor::ROISelect}
    };

}

/**
 * @brief Process a command.
 */
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

/**
 * @brief Parse a command, make sure it is valid JSON and contains the required fields.
 */
json CommandProcessor::ParseCommand(const std::string &command)
{
    /// Parse to ensure it is valid JSON
    json command_json = OpenGuard::Utils::SafeCall([&](){ return json::parse(command);});

    /// Check if the JSON is valid and contains the required fields
    if (command_json.empty() ||command_json.is_null() || !command_json.contains("type") || !command_json.contains("args"))
    {
        Logger::GetInstance().Log("ERROR", "Invalid json received from client: " + command, false);
        return json();
    }

    /// If so, return the parsed JSON
    return command_json;
}

/**
 * @brief Invoke a command.
 */
std::string CommandProcessor::InvokeCommand(const std::string &command_type, const std::string &command_args)
{
    /// If we received an invalid command type, log it and return
    if (command_map.find(command_type) == command_map.end())
    {
        Logger::GetInstance().Log("ERROR", "Invalid command type: " + command_type, false);
        return "";
    }

    /// Otherwise, invoke the command
    return (this->*command_map[command_type])(command_args);
}

/**
 * @brief Validate the arguments of a command, ensuuring all fields are present.
 */
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

/**
 * @brief Update streaming state.
 */
void CommandProcessor::SetStreaming(Stream _stream, bool streaming)
{
    if (_stream == Stream::SNAPSHOT)
    {
        this->snapshot_streaming = streaming;
    }
    else if (_stream == Stream::LOG)
    {
        this->log_streaming = streaming;
    }
}

/**
 * @brief Get the streaming state.
 */
bool CommandProcessor::GetStreaming(Stream _stream)
{
    return _stream == Stream::SNAPSHOT ? this->snapshot_streaming : this->log_streaming;
}

/// Get the hook id for either the snapshot or log hook
HookManager::HookHandle CommandProcessor::GetHookID(Stream stream_hook)
{
    if (stream_hook == CommandProcessor::Stream::SNAPSHOT)
    {
        return this->snapshot_hook_id;
    }
    else if (stream_hook == CommandProcessor::Stream::LOG)
    {
        return this->log_hook_id;
    }

    return 0;
}


std::string CommandProcessor::Login(const std::string &args)
{
    json args_json = ValidateArgs(args, {"username", "password"});

    /// Check the client given username & pw against the one we have stored in the config file.
    if (!args_json.empty() && args_json["username"] == ConfigManager::GetInstance().GetConfig<std::string>("server_username") &&
                              args_json["password"] == ConfigManager::GetInstance().GetConfig<std::string>("server_password"))
    {
        /// If the user is authenticated, send them the config file
        json response = {{"type", "authenticated"}, {"message", ConfigManager::GetInstance().GetFullConfig()}};

        WSServer::GetInstance().Send(response.dump());

        Logger::GetInstance().Log("INFO", "Client authenticated.");

        return "authenticated";
    }

    Logger::GetInstance().Log("ERROR", "Client authentication failed.");

    return "failed";
}

std::string CommandProcessor::PauseSystem(const std::string &args)
{
    json args_json = ValidateArgs(args, {"until"});

    if (args_json.empty())
        return "failed";

    /// Get the duration to pause the system for (unix timestamp)
    int duration = OpenGuard::Utils::SafeCall([&](){ return args_json["until"].get<int>(); });

    /// Bit hacky, ideally we'd like to return this from a hook
    HookManager::GetInstance().AppendOutput("on_motion","{\"pause_system\": " + std::to_string(duration) + "}");

    return "success";
}

std::string CommandProcessor::Snapshot(const std::string &args)
{
    json args_json = ValidateArgs(args, {"status"});

    if (args_json.empty())
        return "failed";

    /// Get the type of request the client asked, either a screenshot or a stream
    auto status = args_json["status"].get<std::string>();
    bool screenshot = status == "screenshot";

    /// if the client asked to stop the snapshot stream, unregister the hook
    if (status == "stop" && snapshot_streaming)
    {
        HookManager::GetInstance().UnregisterHook("on_render", snapshot_hook_id);
        snapshot_streaming = false;
        return "success";
    }

    /// Create a hook that will capture the frame and send it back to the client
    auto snapshot_hook = HookManager::Hook(HookManager::HookType::NATIVE, [screenshot](const std::unordered_map<std::string, std::string>& args) -> std::string
    {
        /// Form the json response
        json response = {{"type", (screenshot ? "screenshot" : "snapshot_stream")}, {"image", args.at("frame")}};

        /// And send it.
        WSServer::GetInstance().Send(response.dump());

        /// From within the hook, make sure to unregister the hook if we're no longer streaming
        if (!CommandProcessor::GetInstance().GetStreaming(Stream::SNAPSHOT))
        {
            auto hook_id = CommandProcessor::GetInstance().GetHookID(Stream::SNAPSHOT);
            HookManager::GetInstance().UnregisterHook("on_render", hook_id);
        }

        return "";
    }, true, 0, screenshot);

    /// If we're not already streaming, register the hook
    if (!snapshot_streaming || status == "screenshot")
    {
        /// Also, if a screenshot is requested, we register the hook as a "one time" hook
        this->snapshot_hook_id = HookManager::GetInstance().RegisterHook("on_render", snapshot_hook);

        if (!screenshot)
            snapshot_streaming = true;
    }

    return "success";
}

std::string CommandProcessor::GetLogs(const std::string &args)
{
    json args_json = ValidateArgs(args, {"type"});

    if (args_json.empty())
        return "failed";

    /// Get the type of request the client asked, either a dump or a stream
    auto log_type = args_json["type"].get<std::string>();

    /// If the client asked for a dump, send the log buffer
    if (log_type == "dump")
    {
        /// If its a dump, send the log buffer
        json response = {{"type", "log_dump"}, {"message", Logger::GetInstance().DumpBuffer()}};
        WSServer::GetInstance().Send(response.dump());
    }
    else if (log_type == "stream_start")
    {
        ///Otherwise, we'll create a hook and register it under the on_log event
        auto log_hook = HookManager::Hook(HookManager::HookType::NATIVE, [](const std::unordered_map<std::string, std::string>& args) -> std::string
        {
            json response = {{"type", "log"}, {"message", args.at("message")}};
            WSServer::GetInstance().Send(response.dump());

            /// If we're no longer supposed to be streaming, unregister the hook
            if (!CommandProcessor::GetInstance().GetStreaming(Stream::LOG))
            {
                auto hook_id = CommandProcessor::GetInstance().GetHookID(Stream::LOG);
                HookManager::GetInstance().UnregisterHook("on_log", hook_id);
            }

            return "";
        }, true, 0, false);

        /// If we're not already streaming, register the hook
        if (!CommandProcessor::GetInstance().GetStreaming(Stream::LOG))
        {
            this->log_hook_id = HookManager::GetInstance().RegisterHook("on_log", log_hook);
            log_streaming = true;
        }
    }
    else if (log_type == "stream_stop")
    {
        /// If the client asked to stop the log stream, unregister the hook
        HookManager::GetInstance().UnregisterHook("on_log", log_hook_id);
        log_streaming = false;
    }

    return "success";
}

std::string CommandProcessor::Restart(const std::string &args)
{
    /// Close the server
    WSServer::GetInstance().CloseServer();
    /// Give time for the server to close
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    /// Restart the program
    OpenGuard::Utils::Restart();
    /// This should never be reached
    return "";
}

std::string CommandProcessor::SetConfig(const std::string &args)
{
    json args_json = ValidateArgs(args, {"config"});

    if (args_json.empty())
        return "failed";

    std::string config = OpenGuard::Utils::SafeCall([args_json]() { return args_json["config"].dump(); });

    /// Overwrite the config file with the one provided by the client
    ConfigManager::GetInstance().OverwriteConfig(config);

    return "success";
}

std::string CommandProcessor::GetVideos(const std::string &args)
{
    json args_json = ValidateArgs(args, {"type"});

    if (args_json.empty())
        return "failed";

    auto video_type = args_json["type"].get<std::string>();

    /// If the client asked for a list of videos
    if (video_type == "list")
    {
        auto video_dir = ConfigManager::GetInstance().GetConfig<std::string>("output_path");
        std::vector<std::string> videos;

        /// Make sure the output directory exists
        if (!std::filesystem::exists(video_dir))
            return "failed";

        /// Iterate over all the videos in the output directory
        for (const auto& entry : std::filesystem::directory_iterator(video_dir))
        {
            videos.push_back(entry.path().filename().string());
        }

        ///
        json response = {{"type", "video_list"}, {"videos", videos}};
        WSServer::GetInstance().Send(response.dump());
    }
    else if (video_type == "stream")
    {
        args_json = ValidateArgs(args, {"video"});

        if (args_json.empty())
            return "failed";

        ///Now, if the client asked to stream a video
        auto video = args_json["video"].get<std::string>();
        auto video_dir = ConfigManager::GetInstance().GetConfig<std::string>("output_path");

        /// If the path exists
        if (std::filesystem::exists(video_dir + "/" + video))
        {
            /// We do this in a thread to avoid halting the main thread till the video is streamed
            std::thread([video, video_dir]()
            {
                std::ifstream video_file(video_dir + "/" + video, std::ios::binary);

                if (!video_file.is_open())
                    return;

                Logger::GetInstance().Log("INFO", "Streaming video: " + video);

                /// Read the video in chunks and send them to the client, 128KB at a time
                const size_t buffer_size = 128 * 1024;
                std::vector<char> buffer(buffer_size);

                /// While we can read from the file and there is data to read
                while (video_file.read(buffer.data(), buffer.size()) || video_file.gcount())
                {
                    auto bytes_read = video_file.gcount();
                    /// If we read some data
                    if (bytes_read > 0)
                    {
                        /// Convert the data to base64 and send it to the client
                        std::string chunk = std::string(buffer.data(), bytes_read);
                        std::string base64_chunk = base64::to_base64(chunk);

                        json response = {{"type", "video_stream"}, {"args", {{"video", video}, {"data", base64_chunk}}}};
                        WSServer::GetInstance().Send(response.dump());

                        /// Avoid flooding the client, sleep for 100ms between each chunk
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                }

                /// Close the file
                video_file.close();

                /// Send a stop signal to the client
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

        /// If the video exists, delete it
        if (std::filesystem::exists(video_dir + "/" + video))
        {
            std::filesystem::remove(video_dir + "/" + video);
        }
    }

    return "success";
}

std::string CommandProcessor::GetHooks(const std::string &args)
{
    /// Make sure the request contains type field
    json args_json = ValidateArgs(args, {"type"});

    if (args_json.empty())
        return "failed";

    /// Extract the request type
    auto request_type = args_json["type"].get<std::string>();

    /// If it is a list,
    if (request_type == "list")
    {
        /// Get a list of all the events there are.
        auto events = HookManager::GetInstance().GetEvents();
        std::vector<std::string> hook_files;

        /// Iterate over all the events
        for (const auto& event : events)
        {
            /// For each of them, get the hooks they contain
            auto hooks = HookManager::GetInstance().GetHooks(event);

            /// Then for each of these hooks we get the script path, this means this only concerns external hooks
            for (const auto& hook : hooks)
            {
                if (hook.type == HookManager::HookType::EXTERNAL)
                {
                    hook_files.push_back(hook.script_path);
                }
            }
        }

        /// Form the response containing all the script paths and send that to the client.
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

        /// If the hook doesn't exist already, register it
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

        if (!hook_file.is_open())
            return "failed";

        /// Read the content of the file and send it to the client
        std::string content((std::istreambuf_iterator<char>(hook_file)), std::istreambuf_iterator<char>());
        hook_file.close();

        json response = {{"type", "get_hooks"}, {"args", {{"type", "content"}, {"content", content}, {"hook", hook}}}};
        WSServer::GetInstance().Send(response.dump());
    }

    return "success";
}

std::string CommandProcessor::ROISelect(const std::string &args)
{
    json args_json = ValidateArgs(args, {"x", "y", "width", "height", "type"});

    if (args_json.empty())
        return "failed";

    /// Extract the ROI selection and form the hook output
    int x = args_json["x"].get<int>();
    int y = args_json["y"].get<int>();
    int width = args_json["width"].get<int>();
    int height = args_json["height"].get<int>();

    auto type = args_json["type"].get<std::string>();
    auto hook_output = type == "set" ? "{\"roi_select\": {\"x\": " + std::to_string(x) + ", \"y\": " + std::to_string(y) + ", \"width\": " + std::to_string(width) + ", \"height\": " + std::to_string(height) + "}}" :
                                       "{\"roi_select\": {\"reset\": true}}";

    /// Hijack the motion event to communicate the ROI selection
    HookManager::GetInstance().AppendOutput("on_motion", hook_output);

    return "success";
}