//
// Created by Nono on 10/02/2025.
//
#include <thread>
#include <filesystem>
#include <iostream>
#include <fstream>

#include "hook_manager.hpp"
#include "../../util/libs/json.hpp"

using json = nlohmann::json;

void HookManager::RegisterHooks(const std::string& hook_path)
{
    if (!std::filesystem::exists(hook_path))
    {
        std::cerr << "⚠️ Hook path does not exist: " << hook_path << std::endl;
        return;
    }

    std::string event_name = std::filesystem::path(hook_path).filename().string();

    for (const auto& script_file : std::filesystem::directory_iterator(hook_path))
    {
        std::string script_path = script_file.path().string();
        std::string ext = script_path.substr(script_path.find_last_of('.') + 1);

        if (ext == "py" /*|| ext == "js" || ext == "sh"*/)
        {
            RegisterHook(event_name, Hook(HookType::EXTERNAL, script_path, IsBlocking(script_path)));
        }
    }
}

void HookManager::RegisterHook(const std::string& event, const Hook& hook)
{
    std::lock_guard<std::mutex> lock(hook_mutex);
    hooks[event].push_back(hook);
}

void HookManager::ExecuteHooks(const std::string& event, std::unordered_map<std::string, std::string>& args)
{
    if (hooks.find(event) == hooks.end())
        return;

    args["event"] = event;

    for (const Hook &hook: hooks[event])
    {
        if (hook.type == HookType::NATIVE)
        {
            if (hook.blocking)
                this->hook_outputs[event] = hook.callback(args);
            else
                std::thread(hook.callback, args).detach();
        }
        else if (hook.type == HookType::EXTERNAL)
        {
            json data = args;
            std::string json_args = data.dump();
            std::string command = "py \"" + hook.script_path + "\" \"" + json_args + "\"";

            if (hook.blocking)
            {
                int result = std::system(command.c_str());
                std::lock_guard<std::mutex> lock(hook_mutex);
                this->hook_outputs[event] = result;
            }
            else
            {
                std::thread([command](){ std::system(command.c_str());}).detach();
            }
        }
    }
}


bool HookManager::IsBlocking(const std::string& hook_path)
{
    std::ifstream file(hook_path);

    if (!file.is_open())
        return false;

    std::string line;
    std::getline(file, line);

    return line.find("BLOCKING") != std::string::npos;
}