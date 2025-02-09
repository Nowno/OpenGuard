#ifndef OPENGUARD_HOOK_MANAGER_HPP
#define OPENGUARD_HOOK_MANAGER_HPP

#include <functional>
#include <string>


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
        Hook(HookType type, const std::function<int(std::vector<std::string>)>& callback, bool blocking = false) : type(type), callback(callback), blocking(blocking) {}
        Hook(HookType type, const std::string& scriptPath, bool blocking = false) : type(type), script_path(scriptPath), blocking(blocking) {}

        HookType type;
        std::function<int(const std::vector<std::string>&)> callback;
        std::string script_path;
        bool blocking;
    };

    HookManager();

    void RegisterHook(const std::string& hook_name, const Hook& hook);
    void ExecuteHooks(const std::string& hook_name, const std::vector<std::string>& args);

    private:
    std::unordered_map<std::string, std::vector<Hook>> hooks;

    bool IsBlocking(const std::string& hook_name);
};


#endif //OPENGUARD_HOOK_MANAGER_HPP
