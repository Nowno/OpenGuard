#include <thread>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <algorithm>


#include "hook_manager.hpp"
#include "../openguard.hpp"
#include "../../utils/utils.hpp"

using json = nlohmann::json;

/**
 * @brief Bulk register external hooks in a given directory.
 */
void HookManager::RegisterHooks(const std::string& hook_path)
{
    /// If the directory does not exist, create it
    if (!std::filesystem::exists(hook_path))
    {
        Logger::GetInstance().Log("WARNING", "Hook path did not exist: " + hook_path);
        std::filesystem::create_directories(hook_path);

        return;
    }

    /// Event name, being the name of the directory
    std::string event_name = std::filesystem::path(hook_path).filename().string();

    /// Prepare a vector to store the hook files to later sort them
    std::vector<std::string> hook_files;

    /// Iterate over the directory and store the script paths
    for (const auto& script_file : std::filesystem::directory_iterator(hook_path))
    {
        std::string script_path = script_file.path().string();
        std::string ext = script_path.substr(script_path.find_last_of('.') + 1);

        if (ext == "py" /*|| ext == "js" || ext == "sh"*/) /// Only Python for now for the sake of simplicity
            hook_files.push_back(script_path);
    }

    /// Sort the hook files to give the user control over the order of execution
    std::sort(hook_files.begin(), hook_files.end());

    /// Register the external hooks
    for (const std::string& hook_file : hook_files)
    {
        RegisterHook(event_name, Hook(HookType::EXTERNAL, hook_file, IsBlocking(hook_file), GetCooldown(hook_file)));
    }
}

/**
 * @brief Individual registers a hook.
 */
void HookManager::RegisterHook(const std::string& event, const Hook& hook)
{
    /// Most commonly used for native hooks.
    std::lock_guard<std::mutex> lock(hook_mutex);
    hooks[event].push_back(hook);
}

/**
 * @brief Execute hooks for a given event.
 */
void HookManager::ExecuteHooks(const std::string& event, std::unordered_map<std::string, std::string> args)
{
    /// If no hooks are registered for the event, return
    if (hooks.find(event) == hooks.end())
        return;

    /// Inject event name and time into args to avoid doing each time
    args["event"] = event;
    args["time"] = std::to_string(time(0));

    /// Reset the output for the event
    hook_outputs[event] = "{}";

    /// Edge case for on_hook event
    for (const Hook& on_hook : hooks["on_hook"])
    {
        try
        {
            /// Reserved this hook for native hooks only to avoid performance issues
            if (on_hook.blocking)
                on_hook.callback(args);
            else
                std::thread(on_hook.callback, args).detach();
        }
        catch (const std::exception& e)
        {
            Logger::GetInstance().Log("ERROR", "Failed to execute on_hook: " + std::string(e.what()));
        }
    }

    for (Hook &hook: hooks[event])
    {
        /// Check if the hook has a cooldown and if it has been executed within the cooldown period
        if (hook.cooldown && time(nullptr) - hook.last_executed < hook.cooldown)
            continue;

        /// If the hook is native, it is as easy as calling the lambda
        if (hook.type == HookType::NATIVE)
        {
            if (hook.blocking)                                      /// In the case of blocking hooks, execute them synchronously and capture the output
                this->hook_outputs[event] = hook.callback(args);
            else
                std::thread(hook.callback, args).detach();          /// For non-blocking we can kind of just run them and forget about them
        }
        else if (hook.type == HookType::EXTERNAL)
        {
            /// Future: For later, maybe have this work with a thread pool or a worker instead.
            /// For now we're running so few scripts that it doesn't matter that much

            /// Same logic as native except we need to handle forming the command to execute the script
            /// Prefix for Python scripts as it may vary
            static std::string python_prefix = ConfigManager::GetInstance().GetConfig<std::string>("python_prefix");

            /// If the prefix is empty, assume Python is in the PATH
            json data = args;

            /// Escape the JSON to ensure it is passed correctly and add it to the command
            std::string command = python_prefix + " " + hook.script_path + " \"" + OpenGuard::Utils::EscapeShell(data.dump()) + "\"";

            if (hook.blocking)
            {
                /// For blocking hooks, execute them synchronously and capture the output
                auto result = OpenGuard::Utils::ExecuteCommand(command);

                std::lock_guard<std::mutex> lock(output_mutex);
                this->hook_outputs[event] = AppendOutput(event, result);
            }
            else
            {
                /// And as before, fire and forget
                std::thread([&, event, command]()
                {
                    std::string result = OpenGuard::Utils::ExecuteCommand(command);

                    std::lock_guard<std::mutex> lock(output_mutex);
                    this->hook_outputs[event] = AppendOutput(event, result);
                }).detach();
            }
        }

        /// Update the last execution time
        hook.last_executed = time(nullptr);
    }

}

/**
 * @brief Get the header of a hook file.
 */
std::string HookManager::GetHookHeader(const std::string& hook_path)
{
    /// If the header is already cached, return it
    if (header_cache.find(hook_path) != header_cache.end())
        return header_cache[hook_path];

    std::ifstream file(hook_path);

    if (!file.is_open())
    {
        Logger::GetInstance().Log("ERROR", "Could not open hook: " + hook_path);
        return "";
    }

    std::string line;
    if (!std::getline(file, line))
    {
        Logger::GetInstance().Log("ERROR", "Could not read header for hook: " + hook_path);
        return "";
    }

    return line; /// Simply return the first line
}

/**
 * @brief Check if a hook is blocking.
 */
bool HookManager::IsBlocking(const std::string& hook_path)
{
    return GetHookHeader(hook_path).find("BLOCKING") != std::string::npos; /// If the first line contains "BLOCKING"
}

/**
 * @brief Get the cooldown of a hook.
 */
int HookManager::GetCooldown(const std::string& hook_path)
{
    std::string header = GetHookHeader(hook_path);

    size_t pos = header.find("COOLDOWN");

    if (pos == std::string::npos)
        return 0;

    /// If the header contains "COOLDOWN", extract the cooldown value, which is a space after "COOLDOWN"
    std::string cooldown = header.substr(pos + 9);

    int cooldown_int = 0;

    try
    {
        /// Attempt to parse the cooldown value
        cooldown_int = std::stoi(cooldown);
    }
    catch (std::invalid_argument& e)
    {
        /// If the cooldown value is not a number, log an error and return the default value
        Logger::GetInstance().Log("ERROR", "Could not parse cooldown for hook: " + hook_path);
    }

    return cooldown_int;
}


/**
 * @brief Get the output of a hook.
 */
std::string HookManager::GetHookOutput(const std::string& event, const std::string& key)
{

    std::lock_guard<std::mutex> lock(output_mutex);

    if (hook_outputs.find(event) == hook_outputs.end())
        return "";

    json data = json::parse(hook_outputs[event]);

    if (data.find(key) == data.end())
        return "";

    return data[key];
}

/**
 * @brief Append output to the output of a hook.
 */
std::string HookManager::AppendOutput(const std::string& event, const std::string& output)
{
    if (hook_outputs.find(event) == hook_outputs.end())
        return output;

    if (output.empty())
        return hook_outputs[event];

    json data;

    try
    {
        data = json::parse(hook_outputs[event]);
        json new_data = json::parse(output);

        for (auto &[key, value]: new_data.items())
        {
            data[key] = value;
        }
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().Log("ERROR", "Failed to append output: " + std::string(e.what()));

        return hook_outputs[event];
    }

    return data.dump();
}
