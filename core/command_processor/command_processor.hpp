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
    enum class Stream
    {
        SNAPSHOT,
        LOG
    };

    static CommandProcessor& GetInstance()
    {
        static CommandProcessor instance;
        return instance;
    }

    /**
     * @brief Parse, validates and invokes a command.
     * @param command The command to process.
     * @return The result of the command.
     */
    std::string Process(const std::string& command);

    /// Set/Get the status of a given stream
    void SetStreaming(Stream _stream, bool streaming);
    bool GetStreaming(Stream _stream);

    /**
     * @brief Get the hook ID for one of the hooks we have here.
     * @param hook (enum) The hook to get the ID for.
     * @return The ID of the hook.
     */
    HookManager::HookHandle GetHookID(CommandProcessor::Stream stream_hook);

    private:
    CommandProcessor();
    CommandProcessor(const CommandProcessor&) = delete;
    CommandProcessor& operator=(const CommandProcessor&) = delete;

    /**
     * @brief Parse a command.
     * @param command The command to parse.
     * @return The parsed command.
     */
    nlohmann::json ParseCommand(const std::string& command);

    /**
     * @brief Validate the arguments of a command.
     * @param args The arguments to validate.
     * @param expected_fields The expected fields in the arguments.
     * @return The validated json.
     */
    nlohmann::json ValidateArgs(const std::string &args, const std::vector<std::string>& expected_fields);

    /**
     * @brief Invoke a command.
     * @param command_type The type of the command.
     * @param command_args The arguments of the command.
     * @return The result of the command.
     */
    std::string InvokeCommand(const std::string& command_type, const std::string& command_args);


    ////////////////////// Commands //////////////////////
    std::string Login(const std::string& args);
    std::string PauseSystem(const std::string& args);
    std::string Snapshot(const std::string& args);
    std::string Restart(const std::string& args);
    std::string GetLogs(const std::string& args);
    std::string SetConfig(const std::string& args);
    std::string GetVideos(const std::string& args);
    std::string GetHooks(const std::string& args);
    std::string ROISelect(const std::string& args);
    /////////////////////////////////////////////////////

    bool snapshot_streaming = false; /// To avoid registering the hook multiple times
    bool log_streaming = false;      /// Idem

    HookManager::HookHandle snapshot_hook_id; /// The hook ID for the snapshot hook
    HookManager::HookHandle log_hook_id;      /// The hook ID for the log hook

    std::unordered_map<std::string, std::string(CommandProcessor::*)(const std::string&)> command_map;
};
#endif //OPENGUARD_COMMAND_PROCESSOR_HPP
