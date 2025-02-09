#ifndef OPENGUARD_USER_CONFIG_HPP
#define OPENGUARD_USER_CONFIG_HPP

#include <string>
#include <unordered_map>

//Hard coded for now

namespace OpenGuard
{
    extern std::unordered_map<std::string, std::string> user_config;  // ✅ Only declare
}

/*
namespace UserConfig
{
    std::unordered_map<std::string, std::string> config;

    void ParseConfig()
    {
        //todo nlohmann json and safe parsing
    }
};
*/


#endif //OPENGUARD_USER_CONFIG_HPP
