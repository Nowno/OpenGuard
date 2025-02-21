#include <thread>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <algorithm>


#include "hook_manager.hpp"
#include "../openguard.hpp"
#include "../../utils/utils.hpp"

using json = nlohmann::json;

void HookManager::RegisterHooks(const std::string& hook_path)
{
    if (!std::filesystem::exists(hook_path))
    {
        Logger::GetInstance().Log("WARNING", "Hook path did not exist: " + hook_path);
        std::filesystem::create_directories(hook_path);

        return;
    }

    std::string event_name = std::filesystem::path(hook_path).filename().string();

    std::vector<std::string> hook_files;

    for (const auto& script_file : std::filesystem::directory_iterator(hook_path))
    {
        std::string script_path = script_file.path().string();
        std::string ext = script_path.substr(script_path.find_last_of('.') + 1);

        if (ext == "py" /*|| ext == "js" || ext == "sh"*/)
            hook_files.push_back(script_path);
    }

    std::sort(hook_files.begin(), hook_files.end());

    for (const std::string& hook_file : hook_files)
    {
        RegisterHook(event_name, Hook(HookType::EXTERNAL, hook_file, IsBlocking(hook_file), GetCooldown(hook_file)));
    }
}

void HookManager::RegisterHook(const std::string& event, const Hook& hook)
{
    std::lock_guard<std::mutex> lock(hook_mutex);
    hooks[event].push_back(hook);
}

void HookManager::ExecuteHooks(const std::string& hook_name, std::unordered_map<std::string, std::string> args)
{
    if (hooks.find(hook_name) == hooks.end())
        return;

    // Inject event name into args
    args["event"] = hook_name;

    // Edge case for on_hook event
    for (const Hook& on_hook : hooks["on_hook"])
    {
        if (on_hook.blocking)
            on_hook.callback(args);
        else
            std::thread(on_hook.callback, args).detach();
    }

    for (Hook &hook: hooks[hook_name])
    {
        if (hook.cooldown && time(nullptr) - hook.last_executed < hook.cooldown)
            continue;

        if (hook.type == HookType::NATIVE)
        {
            if (hook.blocking)
                this->hook_outputs[hook_name] = hook.callback(args);
            else
                std::thread(hook.callback, args).detach();
        }
        else if (hook.type == HookType::EXTERNAL)
        {
            json data = args;

            std::string command = ConfigManager::GetInstance().GetConfig<std::string>("python_prefix") + " " + hook.script_path + " \"" + OpenGuard::Utils::EscapeShell(data.dump()) + "\"";

            if (hook.blocking)
            {
                int result = std::system(command.c_str());
                std::lock_guard<std::mutex> lock(hook_mutex);
                this->hook_outputs[hook_name] = result;
            }
            else
            {
                std::thread([command](){ std::system(command.c_str());}).detach();
            }
        }

        hook.last_executed = time(nullptr);
    }
}

std::string HookManager::GetHookHeader(const std::string& hook_path)
{
    std::ifstream file(hook_path);

    if (!file.is_open())
        return "";

    std::string line;
    std::getline(file, line);

    return line;
}

bool HookManager::IsBlocking(const std::string& hook_path)
{
    return GetHookHeader(hook_path).find("BLOCKING") != std::string::npos;
}

int HookManager::GetCooldown(const std::string& hook_path)
{
    std::string header = GetHookHeader(hook_path);

    size_t pos = header.find("COOLDOWN");

    if (pos == std::string::npos)
        return 0;

    std::string cooldown = header.substr(pos + 9);

    int cooldown_int = 0;

    try
    {
        cooldown_int = std::stoi(cooldown);
    }
    catch (std::invalid_argument& e)
    {
        Logger::GetInstance().Log("ERROR", "Could not parse cooldown for hook: " + hook_path);
    }

    return cooldown_int;
}