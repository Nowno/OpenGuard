#ifndef OPENGUARD_HOOK_MANAGER_HPP
#define OPENGUARD_HOOK_MANAGER_HPP

#include <functional>
#include <string>
#include <mutex>


class HookManager
{
    public:
    enum class HookType
    {
        NATIVE,  /// C++, in the form of a lambda, requires recompilation
        EXTERNAL /// External (for now, only Python but can be extended)
    };


    struct Hook
    {
        /// Constructors for both native and external hooks
        Hook(HookType type, const std::function<int(const std::unordered_map<std::string, std::string>& args)>& callback, bool blocking = false, int cooldown = 0) : type(type), callback(callback), blocking(blocking), cooldown(cooldown) {}
        Hook(HookType type, const std::string& _script_path, bool blocking = false, int cooldown = 0) : type(type), script_path(_script_path), blocking(blocking), cooldown(cooldown) {}

        HookType type;
        std::function<int(const std::unordered_map<std::string, std::string>& args)> callback; /// Exclusive to native hooks
        std::string script_path;                                                               /// Exclusive to external hooks
        bool blocking;
        int cooldown = 0;
        int last_executed = 0;
    };

    /// Singleton access of the HookManager instance
    static HookManager& GetInstance()
    {
        static HookManager instance;
        return instance;
    }

    /**
     * @brief Bulk register external hooks in a given directory.
     * @param hook_path The path to the directory containing the hooks.
     */
    void RegisterHooks(const std::string& hook_folder);

    /**
     * @brief Individual registers a hook.
     * @param hook_name The name of the hook.
     * @param hook The hook to register.
     */
    void RegisterHook(const std::string& event_name, const Hook& hook);

    /**
     * @brief Execute hooks for a given event.
     * @param event The name of the event to execute hooks for.
     * @param args The arguments to pass to the hook
    */
    void ExecuteHooks(const std::string& event, std::unordered_map<std::string, std::string> args);

    private:
    std::unordered_map<std::string, std::vector<Hook>> hooks;  /// Hook table
    std::unordered_map<std::string, std::string> hook_outputs; /// Output of hooks (used for blocking hooks)
    std::unordered_map<std::string, std::string> header_cache; /// Cache for hook headers to avoid reading the same file multiple times
    std::mutex hook_mutex;                                     /// Mutex for hooks operations
    std::mutex output_mutex;                                   /// Mutex for hook_outputs operations

    /// Private constructor for singleton pattern
    HookManager() = default;
    HookManager(const HookManager&) = delete;
    HookManager& operator=(const HookManager&) = delete;

    /**
     * @brief Get the header of a hook script.
     * @param hook_path The path to the hook script.
     * @return The header of the hook script (the first line).
     */
    std::string GetHookHeader(const std::string& hook_path);

    /**
     * @brief Check if a hook is blocking.
     * @param hook_path The path to the hook script.
     * @return Whether the hook is blocking or not (if the first line contains "blocking").
     */
    bool IsBlocking(const std::string& hook_path);

    /**
     * @brief Get the cooldown of a hook.
     * @param hook_path The path to the hook script.
     * @return The cooldown of the hook (if the first line contains "cooldown", if so extract the value).
     */
    int GetCooldown(const std::string& hook_path);

};


#endif //OPENGUARD_HOOK_MANAGER_HPP