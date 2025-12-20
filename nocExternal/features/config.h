#pragma once
#include <string>
#include <vector>

namespace ConfigManager
{
    void CreateConfig(const std::string& name);
    void SaveConfig(const std::string& name);
    void LoadConfig(const std::string& name);
    void DeleteConfig(const std::string& name);
    std::vector<std::string> GetConfigs();
}
