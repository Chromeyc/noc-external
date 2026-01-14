#include "esp.h"
#include "../imgui/imgui.h"
#include "config.h"
#include "../sdk/offset_updater.h"
#include <vector>
#include <string>


#include <Windows.h>

void ActorLoopClass::RenderMenu()
{
    
    static bool key_pressed = false;
    bool key_down = (GetAsyncKeyState(menuToggleKey) & 0x8000) != 0;
    if (key_down && !key_pressed)
    {
        showMenu = !showMenu;
        key_pressed = true;
    }
    else if (!key_down)
    {
        key_pressed = false;
    }
    
    
    ApplyTheme(0); 
    
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(850, 550), ImGuiCond_Always);
    
    if (!showMenu) return;
    
    
    if (ImGui::Begin("noc-external", &showMenu, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
    {
        static int activeTab = 0;

        
        ImGui::BeginChild("Sidebar", ImVec2(180, 0), true);
        {
            ImGui::Spacing();
            ImGui::SetCursorPosX(20);
            ImGui::TextColored(ImVec4(1, 1, 1, 1), "NOC");
            ImGui::SetCursorPosX(20);
            ImGui::TextDisabled("EXTERNAL");
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
            
            
            auto SidebarButton = [&](const char* label, int tabIndex) {
                bool isActive = (activeTab == tabIndex);
                if (isActive) 
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.20f, 0.20f, 1.0f)); 
                
                if (ImGui::Button(label, ImVec2(-1, 35))) 
                    activeTab = tabIndex;
                
                if (isActive) 
                    ImGui::PopStyleColor();
                
                ImGui::Spacing();
            };

            SidebarButton("   ESP", 0);
            SidebarButton("   AIMBOT", 1);
            SidebarButton("   MISC", 2);
            SidebarButton("   SETTINGS", 3);
            
            ImGui::PopStyleVar();
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
        }
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        
        ImGui::BeginChild("Content", ImVec2(0, 0), true);
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));
            ImGui::Spacing();
            
            
            if (activeTab == 0)
            {
                ImGui::Text("ESP Configuration");
                ImGui::Separator();
                ImGui::Spacing();
                
                ImGui::Checkbox("Enable ESP", &settings.enableESP);
                if (settings.enableESP)
                {
                    ImGui::Spacing();
                    
                    ImGui::Columns(2, "ESPCols", false);
                    
                    ImGui::TextDisabled("General");
                    ImGui::Checkbox("Teammates", &settings.showTeam);
                    ImGui::Checkbox("Health Bar", &settings.enableHealthBar);
                    ImGui::Checkbox("Snapline", &settings.enableSnapline);
                    
                    ImGui::NextColumn();
                    
                    ImGui::TextDisabled("Box");
                    ImGui::Checkbox("Box", &settings.enableBox);
                    ImGui::Checkbox("Corner Box", &settings.enableCornerBox);
                    ImGui::Checkbox("Box Outline", &settings.enableBoxOutline);
                    
                    ImGui::Columns(1);
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                    
                    ImGui::TextDisabled("Skeleton");
                    ImGui::Checkbox("Enable Skeleton", &settings.enableSkeleton);
                    if (settings.enableSkeleton)
                    {
                        ImGui::SliderFloat("Thickness##Skel", &settings.skeletonThickness, 1.0f, 5.0f, "%.1f");
                    }
                    
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                    
                    ImGui::TextDisabled("Info");
                    ImGui::Checkbox("Name", &settings.enableName);
                    ImGui::Checkbox("Distance", &settings.enableDistance);
                    
                    ImGui::Spacing();
                    ImGui::SliderFloat("Max Distance", &settings.maxDistance, 100.0f, 5000.0f, "%.0fm");
                    ImGui::SliderFloat("Font Size", &settings.nameSize, 0.5f, 2.0f, "%.1f");

                    ImGui::Spacing();
                    ImGui::TextDisabled("Colors");
                    ImGui::ColorEdit4("Enemy Box", settings.enemyBoxColor, ImGuiColorEditFlags_NoInputs);
                    ImGui::SameLine();
                    ImGui::ColorEdit4("Team Box", settings.teamBoxColor, ImGuiColorEditFlags_NoInputs);
                    
                    ImGui::ColorEdit4("Enemy Skel", settings.enemySkeletonColor, ImGuiColorEditFlags_NoInputs);
                    ImGui::SameLine();
                    ImGui::ColorEdit4("Team Skel", settings.teamSkeletonColor, ImGuiColorEditFlags_NoInputs);
                }
            }
            
            
            if (activeTab == 1)
            {
                ImGui::Text("Aimbot Configuration");
                ImGui::Separator();
                ImGui::Spacing();
                
                ImGui::Checkbox("Enable Aimbot", &aimbot.settings.enableAimbot);
                if (aimbot.settings.enableAimbot)
                {
                    ImGui::Spacing();
                    
                    ImGui::Checkbox("Aim at Head", &aimbot.settings.aimAtHead);
                    ImGui::SameLine();
                    ImGui::Checkbox("Smooth Aim", &aimbot.settings.smoothAim);
                    
                    ImGui::Spacing();
                    ImGui::Checkbox("Draw FOV", &aimbot.settings.showFOVCircle);
                    
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                    
                    ImGui::SliderFloat("FOV", &aimbot.settings.fov, 10.0f, 360.0f, "%.1f");
                    
                    if (aimbot.settings.smoothAim)
                        ImGui::SliderFloat("Smoothness", &aimbot.settings.smoothness, 1.5f, 20.0f, "%.1f");
                    
                    ImGui::SliderFloat("Distance Limit", &aimbot.settings.maxDistance, 50.0f, 2000.0f, "%.0fm");
                        
                    ImGui::Spacing();
                    
                    const char* keys[] = { "Left Mouse", "Right Mouse", "Shift", "Ctrl" };
                    int key_index = 0;
                    if (aimbot.settings.aimKey == VK_LBUTTON) key_index = 0;
                    else if (aimbot.settings.aimKey == VK_RBUTTON) key_index = 1;
                    else if (aimbot.settings.aimKey == VK_SHIFT) key_index = 2;
                    else if (aimbot.settings.aimKey == VK_CONTROL) key_index = 3;
                    
                    if (ImGui::Combo("Aim Key", &key_index, keys, IM_ARRAYSIZE(keys)))
                    {
                        int keys_vk[] = { VK_LBUTTON, VK_RBUTTON, VK_SHIFT, VK_CONTROL };
                        aimbot.settings.aimKey = keys_vk[key_index];
                    }
                    
                    ImGui::Spacing();
                    ImGui::ColorEdit4("FOV Color", aimbot.settings.fovCircleColor);
                }
            }
            
            
            if (activeTab == 2)
            {
                ImGui::Text("Miscellaneous");
                ImGui::Separator();
                ImGui::Spacing();
                
                ImGui::Checkbox("Speed", &misc.settings.enableSpeed);
                if (misc.settings.enableSpeed)
                    ImGui::SliderFloat("Speed##SpeedValue", &misc.settings.speed, 1.0f, 200.0f, "%.1f");
                    
                ImGui::Spacing();
                
                ImGui::Checkbox("Jump Power", &misc.settings.enableJumpPower);
                if (misc.settings.enableJumpPower)
                    ImGui::SliderFloat("Power##JumpValue", &misc.settings.jumpPower, 0.0f, 200.0f, "%.1f");
            }
            
            
            if (activeTab == 3)
            {
                ImGui::Text("System Settings");
                ImGui::Separator();
                ImGui::Spacing();
                
                const char* keys[] = { "TAB", "INSERT", "F1", "F2", "F3" };
                int key_index = 0;
                if (menuToggleKey == VK_TAB) key_index = 0;
                else if (menuToggleKey == VK_INSERT) key_index = 1;
                else if (menuToggleKey == VK_F1) key_index = 2;
                else if (menuToggleKey == VK_F2) key_index = 3;
                else if (menuToggleKey == VK_F3) key_index = 4;
                
                if (ImGui::Combo("Menu Toggle Key", &key_index, keys, IM_ARRAYSIZE(keys)))
                {
                    int keys_vk[] = { VK_TAB, VK_INSERT, VK_F1, VK_F2, VK_F3 };
                    menuToggleKey = keys_vk[key_index];
                }
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::TextDisabled("Configuration");
                
                static char configName[64] = "";
                static int selectedConfig = -1;
                static std::vector<std::string> configList = ConfigManager::GetConfigs();
                
                ImGui::InputText("Name", configName, 64);
                if (ImGui::Button("Create"))
                {
                    ConfigManager::CreateConfig(configName);
                    configList = ConfigManager::GetConfigs();
                }
                ImGui::SameLine();
                if (ImGui::Button("Refresh"))
                {
                    configList = ConfigManager::GetConfigs();
                }
                
                ImGui::Spacing();
                
                if (ImGui::BeginListBox("Configs", ImVec2(-FLT_MIN, 100)))
                {
                    for (int i = 0; i < configList.size(); i++)
                    {
                        const bool is_selected = (selectedConfig == i);
                        if (ImGui::Selectable(configList[i].c_str(), is_selected))
                            selectedConfig = i;

                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndListBox();
                }
                
                ImGui::Spacing();
                
                if (ImGui::Button("Load") && selectedConfig >= 0 && selectedConfig < configList.size())
                {
                    ConfigManager::LoadConfig(configList[selectedConfig]);
                }
                ImGui::SameLine();
                if (ImGui::Button("Save") && selectedConfig >= 0 && selectedConfig < configList.size())
                {
                    ConfigManager::SaveConfig(configList[selectedConfig]);
                }
                ImGui::SameLine();
                if (ImGui::Button("Delete") && selectedConfig >= 0 && selectedConfig < configList.size())
                {
                    ConfigManager::DeleteConfig(configList[selectedConfig]);
                    configList = ConfigManager::GetConfigs();
                    selectedConfig = -1;
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::TextDisabled("System");
                ImGui::SameLine();
                if (ImGui::Button("Update Offsets"))
                {
                    if (OffsetUpdater::UpdateOffsets())
                    {
                        MessageBoxA(NULL, "Offsets updated successfully!", "Update", MB_OK | MB_ICONINFORMATION);
                    }
                    else
                    {
                        MessageBoxA(NULL, "Failed to update offsets.", "Update Error", MB_OK | MB_ICONERROR);
                    }
                }
            }
            ImGui::PopStyleVar();
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

void ActorLoopClass::ApplyTheme(int theme)
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.WindowRounding = 4.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 2.0f;
    style.PopupRounding = 2.0f;
    style.ScrollbarRounding = 2.0f;
    style.GrabRounding = 2.0f;
    style.TabRounding = 2.0f;
    style.WindowPadding = ImVec2(10, 10);

    
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    
    
    colors[ImGuiCol_WindowBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.98f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.04f, 0.04f, 0.04f, 0.98f);
    
    colors[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    
    colors[ImGuiCol_FrameBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    
    colors[ImGuiCol_TitleBg] = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.02f, 0.02f, 0.02f, 0.75f);
    
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
    
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    
    colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    
    colors[ImGuiCol_SliderGrab] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    
    colors[ImGuiCol_Button] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    
    colors[ImGuiCol_Header] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    
    colors[ImGuiCol_Separator] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.20f, 0.20f, 0.20f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.30f, 0.30f, 0.30f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.40f, 0.40f, 0.95f);
    
    colors[ImGuiCol_Tab] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.20f, 0.20f, 0.20f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.30f, 0.30f, 0.30f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}
