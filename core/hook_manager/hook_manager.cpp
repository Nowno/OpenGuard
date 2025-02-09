//
// Created by Nono on 10/02/2025.
//

#include "hook_manager.hpp"
#include <thread>
#include <filesystem>
#include <iostream>

HookManager::HookManager()
{
    const std::string path = "./hooks";
    if (!std::filesystem::exists(path))
    {
        std::filesystem::create_directory(path);
    }

    for (const auto & entry : std::filesystem::directory_iterator(path))
    {
        std::cout << entry.path() << std::endl;
    }
}

void HookManager::RegisterHook(const std::string& hook_name, const Hook& hook)
{
    hooks[hook_name].push_back(hook);
}

void HookManager::ExecuteHooks(const std::string& hook_name, const std::vector<std::string>& args)
{
    if (hooks.find(hook_name) == hooks.end())
        return;

    for (const Hook &hook: hooks[hook_name])
    {
        if (hook.type == HookType::NATIVE)
        {
            if (hook.blocking)
                hook.callback(args);
            else
                std::thread(hook.callback, args).detach();
        }
        else
        {

        }
    }
}