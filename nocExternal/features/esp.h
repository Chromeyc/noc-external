#pragma once
#ifndef ACTOR_LOOP_HEADER
#define ACTOR_LOOP_HEADER

#include <windows.h>
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <unordered_map>
#include "../imgui/imgui.h"
#include "../sdk/memory.h"
#include "../sdk/sdk.h"
#include "aimbot.h"
#include "misc.h"

// Forward declaration of Overlay
class Overlay;

struct Player {
    uintptr_t address = 0;
    std::string name = "";
    Vector3 position = { 0, 0, 0 };
    Vector3 size = { 0, 0, 0 };
    bool valid = false;
    bool isTeammate = false;
    uintptr_t teamAddress = 0;
    uintptr_t characterAddress = 0;
    float health = 0.0f;
    float maxHealth = 0.0f;
};

struct ESPSettings {
    bool enableESP = true;
    bool enableBox = true;
    bool enableName = true;
    bool enableDistance = true;
    bool showTeam = true;
    float maxDistance = 1000.0f;
    float enemyBoxColor[4] = { 0.0f, 0.831f, 1.0f, 1.0f };
    float enemyNameColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float enemySnaplineColor[4] = { 0.0f, 0.831f, 1.0f, 1.0f };
    float enemyDistanceColor[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
    float teamBoxColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    float teamNameColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float teamSnaplineColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    float teamDistanceColor[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
    float boxThickness = 2.0f;
    float snaplineThickness = 2.0f;
    int snaplineType = 0;
    bool enableSnapline = false;
    bool enableHealthBar = false;
    bool enableBoxOutline = false;
    bool enableCornerBox = false;
    float boxOutlineThickness = 1.0f;
    float nameSize = 1.0f;
    bool enableSkeleton = false;
    float skeletonThickness = 1.5f;
    float enemySkeletonColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float teamSkeletonColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
};

class ActorLoopClass
{
public:
    ActorLoopClass();
    ~ActorLoopClass() = default;

    bool Initialize();
    void Tick(); 
    void Render();
    void Render(Overlay* overlay);
    void RenderMenu();

    const std::vector<Player>& GetPlayers();
    const Matrix4x4& GetCachedViewMatrix();

    bool WorldToScreen(const Vector3& world_pos, Vector2D& screen_pos, const Matrix4x4& view_matrix);
    
    Vector3 GetCameraPosition();
    Vector3 GetCameraRotation();
    void SetCameraRotation(const Vector3& rotation);
    
    void UpdateAimbot();
    void UpdateMisc();
    bool GetBestTargetPlayer(Player& out_player); 
    float GetDistance(const Vector3& pos1, const Vector3& pos2);
    float GetFOV(const Vector3& target_pos);
    Vector3 CalculateAngle(const Vector3& from, const Vector3& to);
    
    uintptr_t GetLocalHumanoid();
    uintptr_t GetWorkspace();
    uintptr_t GetDataModel();
    uintptr_t GetLocalPlayerTeam();
    
    // Feature components
    ESPSettings settings;
    Aimbot aimbot;
    Misc misc;

    // Friendship for component access
    friend class Aimbot;
    friend class Misc;

    // State helpers
    bool IsMenuVisible() const { return showMenu; }
    uintptr_t GetLocalPlayer() const { return local_player; }
    
    // UI Helpers
    void ApplyTheme(int theme);
    int currentTheme = 0;
    int menuToggleKey = VK_INSERT;

    // Drawing Logic (Public)
    void DrawCornerBox(ImDrawList* draw_list, float min_x, float min_y, float max_x, float max_y, ImU32 color, float thickness);
    void DrawBox(ImDrawList* draw_list, float min_x, float min_y, float max_x, float max_y, ImU32 color, float thickness);
    void DrawSnapline(ImDrawList* draw_list, float x, float y, ImU32 color, float thickness, int type);
    void DrawSkeleton(ImDrawList* draw_list, uintptr_t character, ImU32 color, float thickness);
    Vector3 GetBonePosition(uintptr_t character, const std::string& bone_name);
    void DrawFOVCircle(ImDrawList* draw_list, float fov, ImU32 color);

    // Core Data (Public for ease of use in features)
    std::unique_ptr<Memory> memory;
    uintptr_t game_base = 0;
    uintptr_t local_player = 0;
    bool showMenu = false;

    // Performance Caching
    Matrix4x4 cached_view_matrix;
    Vector3 cached_camera_pos = {0,0,0};
    std::vector<Player> cached_players;

    // Bone Caching
    std::unordered_map<uintptr_t, std::unordered_map<std::string, uintptr_t>> bone_cache;
    std::unordered_map<uintptr_t, std::chrono::high_resolution_clock::time_point> bone_cache_time;
    std::unordered_map<uintptr_t, std::unordered_map<std::string, Vector3>> bone_pos_cache;

    // Core Caching
    uintptr_t cached_datamodel = 0;
    uintptr_t cached_workspace = 0;
    std::chrono::high_resolution_clock::time_point last_core_refresh;

private:
    // Internal Utilities
    std::string ReadString(uintptr_t address);
    std::string LengthReadString(uintptr_t string);
    std::string GetInstanceName(uintptr_t instance_address);
    std::string GetInstanceClassName(uintptr_t instance_address);
    
    uintptr_t FindFirstChild(uintptr_t instance_address, const std::string& child_name);
    uintptr_t FindFirstChildByClass(uintptr_t instance_address, const std::string& class_name);
};

// Singleton pattern handled in main.cpp, accessed via extern here
extern std::unique_ptr<ActorLoopClass> ActorLoop;

#endif // ACTOR_LOOP_HEADER