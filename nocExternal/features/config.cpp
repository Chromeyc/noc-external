#include "config.h"
#include "esp.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

namespace ConfigManager
{
    std::string GetConfigDirectory()
    {
        std::string path = "Configs";
        if (!fs::exists(path))
            fs::create_directory(path);
        return path;
    }

    void CreateConfig(const std::string& name)
    {
        SaveConfig(name);
    }

    void SaveConfig(const std::string& name)
    {
        if (!ActorLoop) return;
        
        std::string path = GetConfigDirectory() + "/" + name + ".cfg";
        std::ofstream file(path);
        if (!file.is_open()) return;

        auto& s = ActorLoop->settings;
        auto& a = ActorLoop->aimbot.settings;
        auto& m = ActorLoop->misc.settings;

        
        file << "ESP_Enable=" << s.enableESP << "\n";
        file << "ESP_Box=" << s.enableBox << "\n";
        file << "ESP_Name=" << s.enableName << "\n";
        file << "ESP_Distance=" << s.enableDistance << "\n";
        file << "ESP_Team=" << s.showTeam << "\n";
        file << "ESP_MaxDist=" << s.maxDistance << "\n";
        file << "ESP_BoxThick=" << s.boxThickness << "\n";
        file << "ESP_SnapLine=" << s.enableSnapline << "\n";
        file << "ESP_HealthBar=" << s.enableHealthBar << "\n";
        file << "ESP_Corner=" << s.enableCornerBox << "\n";
        file << "ESP_Outline=" << s.enableBoxOutline << "\n";
        
        file << "ESP_EBoxColor=" << s.enemyBoxColor[0] << "," << s.enemyBoxColor[1] << "," << s.enemyBoxColor[2] << "," << s.enemyBoxColor[3] << "\n";
        file << "ESP_ENameColor=" << s.enemyNameColor[0] << "," << s.enemyNameColor[1] << "," << s.enemyNameColor[2] << "," << s.enemyNameColor[3] << "\n";
        file << "ESP_TBoxColor=" << s.teamBoxColor[0] << "," << s.teamBoxColor[1] << "," << s.teamBoxColor[2] << "," << s.teamBoxColor[3] << "\n";
        
        
        file << "Aim_Enable=" << a.enableAimbot << "\n";
        file << "Aim_Head=" << a.aimAtHead << "\n";
        file << "Aim_Smooth=" << a.smoothAim << "\n";
        file << "Aim_FOV=" << a.fov << "\n";
        file << "Aim_SmoothVal=" << a.smoothness << "\n";
        file << "Aim_Key=" << a.aimKey << "\n";
        file << "Aim_MaxDist=" << a.maxDistance << "\n";
        file << "Aim_ShowCircle=" << a.showFOVCircle << "\n";
        
        
        file << "Misc_Speed=" << m.enableSpeed << "\n";
        file << "Misc_SpeedVal=" << m.speed << "\n";
        file << "Misc_Gravity=" << m.enableGravity << "\n";
        file << "Misc_GravityVal=" << m.gravity << "\n";
        file << "Misc_Jump=" << m.enableJumpPower << "\n";
        file << "Misc_JumpVal=" << m.jumpPower << "\n";
        
        
        file << "Sys_Theme=" << ActorLoop->currentTheme << "\n";
        file << "Sys_MenuKey=" << ActorLoop->menuToggleKey << "\n";
        
        file.close();
    }

    void LoadConfig(const std::string& name)
    {
        if (!ActorLoop) return;
        
        std::string path = GetConfigDirectory() + "/" + name + ".cfg";
        std::ifstream file(path);
        if (!file.is_open()) return;

        auto& s = ActorLoop->settings;
        auto& a = ActorLoop->aimbot.settings;
        auto& m = ActorLoop->misc.settings;

        std::string line;
        while (std::getline(file, line))
        {
            size_t delimiterPos = line.find('=');
            if (delimiterPos == std::string::npos) continue;

            std::string key = line.substr(0, delimiterPos);
            std::string value = line.substr(delimiterPos + 1);

            try {
                
                if (key == "ESP_Enable") s.enableESP = std::stoi(value);
                else if (key == "ESP_Box") s.enableBox = std::stoi(value);
                else if (key == "ESP_Name") s.enableName = std::stoi(value);
                else if (key == "ESP_Distance") s.enableDistance = std::stoi(value);
                else if (key == "ESP_Team") s.showTeam = std::stoi(value);
                else if (key == "ESP_MaxDist") s.maxDistance = std::stof(value);
                else if (key == "ESP_BoxThick") s.boxThickness = std::stof(value);
                else if (key == "ESP_SnapLine") s.enableSnapline = std::stoi(value);
                else if (key == "ESP_HealthBar") s.enableHealthBar = std::stoi(value);
                else if (key == "ESP_Corner") s.enableCornerBox = std::stoi(value);
                else if (key == "ESP_Outline") s.enableBoxOutline = std::stoi(value);

                
                else if (key.find("Color") != std::string::npos)
                {
                    float r, g, b, alpha;
                    char comma;
                    std::stringstream ss(value);
                    ss >> r >> comma >> g >> comma >> b >> comma >> alpha;
                    
                    if (key == "ESP_EBoxColor") { s.enemyBoxColor[0]=r; s.enemyBoxColor[1]=g; s.enemyBoxColor[2]=b; s.enemyBoxColor[3]=alpha; }
                    else if (key == "ESP_ENameColor") { s.enemyNameColor[0]=r; s.enemyNameColor[1]=g; s.enemyNameColor[2]=b; s.enemyNameColor[3]=alpha; }
                    else if (key == "ESP_TBoxColor") { s.teamBoxColor[0]=r; s.teamBoxColor[1]=g; s.teamBoxColor[2]=b; s.teamBoxColor[3]=alpha; }
                }

                
                else if (key == "Aim_Enable") a.enableAimbot = std::stoi(value);
                else if (key == "Aim_Head") a.aimAtHead = std::stoi(value);
                else if (key == "Aim_Smooth") a.smoothAim = std::stoi(value);
                else if (key == "Aim_FOV") a.fov = std::stof(value);
                else if (key == "Aim_SmoothVal") a.smoothness = std::stof(value);
                else if (key == "Aim_Key") a.aimKey = std::stoi(value);
                else if (key == "Aim_MaxDist") a.maxDistance = std::stof(value);
                else if (key == "Aim_ShowCircle") a.showFOVCircle = std::stoi(value);

                
                else if (key == "Misc_Speed") m.enableSpeed = std::stoi(value);
                else if (key == "Misc_SpeedVal") m.speed = std::stof(value);
                else if (key == "Misc_Gravity") m.enableGravity = std::stoi(value);
                else if (key == "Misc_GravityVal") m.gravity = std::stof(value);
                else if (key == "Misc_Jump") m.enableJumpPower = std::stoi(value);
                else if (key == "Misc_JumpVal") m.jumpPower = std::stof(value);
                
                
                else if (key == "Sys_Theme") { ActorLoop->currentTheme = std::stoi(value); ActorLoop->ApplyTheme(ActorLoop->currentTheme); }
                else if (key == "Sys_MenuKey") ActorLoop->menuToggleKey = std::stoi(value);

            } catch (...) {}
        }
    }
    
    void DeleteConfig(const std::string& name)
    {
        std::string path = GetConfigDirectory() + "/" + name + ".cfg";
        if (fs::exists(path))
            fs::remove(path);
    }

    std::vector<std::string> GetConfigs()
    {
        std::vector<std::string> configs;
        std::string path = GetConfigDirectory();
        for (const auto& entry : fs::directory_iterator(path))
        {
            if (entry.path().extension() == ".cfg")
            {
                configs.push_back(entry.path().stem().string());
            }
        }
        return configs;
    }
}
