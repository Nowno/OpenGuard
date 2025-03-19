#ifndef OPENGUARD_COMMAND_PROCESSOR_HPP
#define OPENGUARD_COMMAND_PROCESSOR_HPP

#include <string>
#include <json/json.hpp>
#include <unordered_map>

#include "../hook_manager/hook_manager.hpp"

class CommandProcessor
{
    public:
    ///enum for hook type
    enum class Hooks
    {
        SNAPSHOT,
        LOG
    };

    static CommandProcessor& GetInstance()
    {
        static CommandProcessor instance;
        return instance;
    }

    std::string Process(const std::string& command);

    void SetStreaming(const std::string& stream, bool streaming);
    bool GetStreaming(const std::string& stream);

    HookManager::HookHandle GetHookID(Hooks hook);

    private:
    CommandProcessor();
    CommandProcessor(const CommandProcessor&) = delete;
    CommandProcessor& operator=(const CommandProcessor&) = delete;

    nlohmann::json ParseCommand(const std::string& command);
    nlohmann::json ValidateArgs(const std::string &args, const std::vector<std::string>& expected_fields);

    std::string InvokeCommand(const std::string& command_type, const std::string& command_args);


    ////////////////////// Commands //////////////////////
    std::string Login(const std::string& args);
    std::string PauseSystem(const std::string& args);
    std::string Snapshot(const std::string& args);
    std::string Restart(const std::string& args);
    std::string GetLogs(const std::string& args);

    bool snapshot_streaming = false; /// To avoid registering the hook multiple times
    bool log_streaming = false;      /// Idem

    HookManager::HookHandle snapshot_hook_id;
    HookManager::HookHandle log_hook_id;

    std::unordered_map<std::string, std::string (CommandProcessor::*)(const std::string&)> command_map;
};
#endif //OPENGUARD_COMMAND_PROCESSOR_HPP
