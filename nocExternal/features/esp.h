#pragma once
#include "aimbot.h"
#include "misc.h"
#include "../sdk/memory.h"
#include "../sdk/sdk.h"
#include <vector>
#include <string>
#include <memory>
#include <chrono>

class Overlay;
struct ImDrawList;
struct ImVec2;
typedef unsigned int ImU32;

struct Player {
    uintptr_t address;
    std::string name;
    Vector3 position;
    Vector3 size;
    bool valid;
    bool isTeammate;
    uintptr_t teamAddress;
    uintptr_t characterAddress;
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
    bool showOnlyVisible = false;
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
    bool Initialize();
   
    void Render();                  
    void Render(Overlay* overlay);
    void RenderMenu();

    std::vector<Player> GetPlayers();
    Matrix4x4 GetViewMatrix();

    bool WorldToScreen(const Vector3& world_pos, Vector2D& screen_pos, const Matrix4x4& view_matrix);
    
    Vector3 GetCameraPosition();
    Vector3 GetCameraRotation();
    void SetCameraRotation(const Vector3& rotation);
    
    void UpdateAimbot();
    void UpdateMisc();
    Player* GetBestTarget();
    float GetDistance(const Vector3& pos1, const Vector3& pos2);
    float GetFOV(const Vector3& target_pos);
    Vector3 CalculateAngle(const Vector3& from, const Vector3& to);
    Vector3 SmoothAngle(const Vector3& current, const Vector3& target, float smoothness);
    
    uintptr_t GetLocalHumanoid();
    uintptr_t GetWorkspace();
    uintptr_t GetLocalPlayerTeam();
    bool IsTeammate(uintptr_t player_address, uintptr_t local_team_address);
    
    ESPSettings settings;
    
    Aimbot aimbot;
    Misc misc;
    
    friend class Aimbot;
    friend class Misc;
    bool IsMenuVisible() const { return showMenu; }
    
    uintptr_t GetLocalPlayer() const { return local_player; }
    
    int currentTheme = 0;
    int menuToggleKey = VK_INSERT;
    void ApplyTheme(int theme);

private:
    std::unique_ptr<Memory> memory;
    uintptr_t game_base;
    uintptr_t local_player;
    bool showMenu = false;

    void DrawCornerBox(ImDrawList* draw_list, float min_x, float min_y, float max_x, float max_y, ImU32 color, float thickness);
    void DrawBox(ImDrawList* draw_list, float min_x, float min_y, float max_x, float max_y, ImU32 color, float thickness);
    void DrawSnapline(ImDrawList* draw_list, float x, float y, ImU32 color, float thickness, int type);
    void DrawSkeleton(ImDrawList* draw_list, uintptr_t character, ImU32 color, float thickness);
    Vector3 GetBonePosition(uintptr_t character, const std::string& bone_name);
    void DrawFOVCircle(ImDrawList* draw_list, float fov, ImU32 color);
    
    std::string ReadString(uintptr_t address);
    std::string LengthReadString(uintptr_t string);
    std::string GetInstanceName(uintptr_t instance_address);
    std::string GetInstanceClassName(uintptr_t instance_address);
    
    uintptr_t FindFirstChild(uintptr_t instance_address, const std::string& child_name);
    uintptr_t FindFirstChildByClass(uintptr_t instance_address, const std::string& class_name);
};

extern std::unique_ptr<ActorLoopClass> ActorLoop;