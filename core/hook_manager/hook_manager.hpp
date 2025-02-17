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
        NATIVE,
        EXTERNAL
    };

    struct Hook
    {
        Hook(HookType type, const std::function<int(const std::unordered_map<std::string, std::string>& args)>& callback, bool blocking = false) : type(type), callback(callback), blocking(blocking) {}
        Hook(HookType type, const std::string& scriptPath, bool blocking = false) : type(type), script_path(scriptPath), blocking(blocking) {}

        HookType type;
        std::function<int(const std::unordered_map<std::string, std::string>& args)> callback;
        std::string script_path;
        bool blocking;
    };

    // Singleton access
    static HookManager& GetInstance()
    {
        static HookManager instance;
        return instance;
    }

    void RegisterHooks(const std::string& hook_path);
    void RegisterHook(const std::string& hook_name, const Hook& hook);
    void ExecuteHooks(const std::string& hook_name, std::unordered_map<std::string, std::string> args);

    private:
    std::unordered_map<std::string, std::vector<Hook>> hooks;
    std::unordered_map<std::string, int> hook_outputs;
    std::mutex hook_mutex;
    std::mutex output_mutex;

    HookManager() = default;
    HookManager(const HookManager&) = delete;
    HookManager& operator=(const HookManager&) = delete;

    bool IsBlocking(const std::string& hook_name);


};


#endif //OPENGUARD_HOOK_MANAGER_HPP