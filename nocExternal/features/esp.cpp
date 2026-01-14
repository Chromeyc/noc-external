#define _CRT_SECURE_NO_WARNINGS
#include "esp.h"
#include <iostream>
#include <chrono>
#include <unordered_map>
#include <immintrin.h> 
#include <float.h>
#include <windows.h>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "../imgui/imgui.h"
#include "../overlay/overlay.h"
#include "../sdk/offsets.h"
#include "../sdk/offset_updater.h"
#include "config.h"

// VERIFIED_ESP_CPP_ULTRA_OPTIMIZED
template<typename T>
static T clamp_v(T val, T min, T max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

static ImU32 ColorToImU32(float* color)
{
    return IM_COL32((int)(color[0] * 255.0f), (int)(color[1] * 255.0f), (int)(color[2] * 255.0f), (int)(color[3] * 255.0f));
}

struct PlayerCache {
    uintptr_t address = 0;
    uintptr_t character = 0;
    uintptr_t root_part_primitive = 0;
    uintptr_t humanoid = 0;
    std::string name = "";
    uintptr_t team = 0;
    bool isTeammate = false;
    std::unordered_map<std::string, uintptr_t> bone_primitives;
};

static std::vector<PlayerCache> player_entities;
static std::chrono::high_resolution_clock::time_point last_entity_refresh;
static std::string temp_string;

ActorLoopClass::ActorLoopClass() 
{
    memory = std::make_unique<Memory>();
    game_base = 0;
    local_player = 0;
    player_entities.reserve(100);
    cached_players.reserve(100);
    cached_view_matrix = {};
}

bool ActorLoopClass::Initialize() 
{
    if (!memory) return false;
    if (!memory->AttachToProcess("RobloxPlayerBeta.exe"))
    {
        if (!memory->AttachToProcess("RobloxPlayer.exe"))
        {
            return false;
        }
    }
   
    game_base = memory->GetBaseAddress();
    return (game_base != 0);
}

std::string ActorLoopClass::ReadString(uintptr_t address)
{
    temp_string.clear();
    char buffer[32] = {0};
    if (memory->ReadMemory<char>(address) == 0) return "???";
    
    // Read in one block for speed
    for(int i = 0; i < 32; i++) {
        char c = memory->ReadMemory<char>(address + i);
        if (c == 0) break;
        temp_string.push_back(c);
    }
    return temp_string;
}

std::string ActorLoopClass::LengthReadString(uintptr_t string)
{
    const auto length = memory->ReadMemory<int>(string + Offsets::Misc::StringLength);
    if (length >= 16u) {
        uintptr_t ptr = memory->ReadMemory<uintptr_t>(string);
        if (ptr) return ReadString(ptr);
    }
    return ReadString(string);
}

std::string ActorLoopClass::GetInstanceName(uintptr_t instance_address)
{
    const auto _get = memory->ReadMemory<uintptr_t>(instance_address + Offsets::Instance::Name);
    if (_get) return LengthReadString(_get);
    return "???";
}

std::string ActorLoopClass::GetInstanceClassName(uintptr_t instance_address)
{
    const auto class_desc = memory->ReadMemory<uintptr_t>(instance_address + Offsets::Instance::ClassDescriptor);
    if (!class_desc) return "???";
    const auto class_name_ptr = memory->ReadMemory<uintptr_t>(class_desc + Offsets::Instance::ClassName);
    if (class_name_ptr) return ReadString(class_name_ptr);
    return "???";
}

uintptr_t ActorLoopClass::FindFirstChild(uintptr_t instance_address, const std::string& child_name)
{
    if (!instance_address) return 0;
    uintptr_t children_container = memory->ReadMemory<uintptr_t>(instance_address + Offsets::Instance::ChildrenStart);
    if (!children_container) return 0;
    uintptr_t start = memory->ReadMemory<uintptr_t>(children_container);
    uintptr_t end = memory->ReadMemory<uintptr_t>(children_container + 8);
    if (!start || !end || start >= end || (end - start) > 2000 * 16) return 0;

    for (uintptr_t current = start; current < end; current += 16)
    {
        auto child = memory->ReadMemory<uintptr_t>(current);
        if (child && GetInstanceName(child) == child_name) return child;
    }
    return 0;
}

uintptr_t ActorLoopClass::FindFirstChildByClass(uintptr_t instance_address, const std::string& class_name)
{
    if (!instance_address) return 0;
    uintptr_t children_container = memory->ReadMemory<uintptr_t>(instance_address + Offsets::Instance::ChildrenStart);
    if (!children_container) return 0;
    uintptr_t start = memory->ReadMemory<uintptr_t>(children_container);
    uintptr_t end = memory->ReadMemory<uintptr_t>(children_container + 8);
    if (!start || !end || start >= end || (end - start) > 2000 * 16) return 0;

    for (uintptr_t current = start; current < end; current += 16)
    {
        auto child = memory->ReadMemory<uintptr_t>(current);
        if (child && GetInstanceClassName(child) == class_name) return child;
    }
    return 0;
}

uintptr_t ActorLoopClass::GetDataModel()
{
    auto now = std::chrono::high_resolution_clock::now();
    if (cached_datamodel && now - last_core_refresh < std::chrono::seconds(10)) return cached_datamodel;

    uintptr_t datamodel = 0;
    uintptr_t fake_datamodel = memory->ReadMemory<uintptr_t>(game_base + Offsets::FakeDataModel::Pointer);
    if (fake_datamodel) datamodel = memory->ReadMemory<uintptr_t>(fake_datamodel + Offsets::FakeDataModel::RealDataModel);
    
    if (!datamodel)
    {
        uintptr_t visual_engine = memory->ReadMemory<uintptr_t>(game_base + Offsets::VisualEngine::Pointer);
        if (visual_engine)
        {
            uintptr_t ptr1 = memory->ReadMemory<uintptr_t>(visual_engine + Offsets::VisualEngine::ToDataModel1);
            if (ptr1) datamodel = memory->ReadMemory<uintptr_t>(ptr1 + Offsets::VisualEngine::ToDataModel2);
        }
    }

    if (datamodel) {
        cached_datamodel = datamodel;
        last_core_refresh = now;
    }
    return datamodel;
}

void ActorLoopClass::Tick()
{
    if (!memory) return;
    bone_pos_cache.clear(); 
    auto now = std::chrono::high_resolution_clock::now();
    
    if (player_entities.empty() || now - last_entity_refresh > std::chrono::seconds(3))
    {
        player_entities.clear();
        uintptr_t datamodel = GetDataModel();
        if (datamodel)
        {
            uintptr_t players_service = FindFirstChildByClass(datamodel, "Players");
            if (players_service)
            {
                local_player = memory->ReadMemory<uintptr_t>(players_service + Offsets::Player::LocalPlayer);
                uintptr_t local_team = GetLocalPlayerTeam();
                uintptr_t container = memory->ReadMemory<uintptr_t>(players_service + Offsets::Instance::ChildrenStart);
                if (container)
                {
                    uintptr_t start = memory->ReadMemory<uintptr_t>(container);
                    uintptr_t end = memory->ReadMemory<uintptr_t>(container + 8);
                    
                    if (start && end && (end - start) < 1000 * 16)
                    {
                        for (uintptr_t current = start; current < end; current += 16)
                        {
                            uintptr_t player_addr = memory->ReadMemory<uintptr_t>(current);
                            if (!player_addr || player_addr == local_player) continue;

                            uintptr_t character = memory->ReadMemory<uintptr_t>(player_addr + Offsets::Player::ModelInstance);
                            if (!character) continue;

                            uintptr_t root_part = FindFirstChild(character, "HumanoidRootPart");
                            if (!root_part) continue;

                            uintptr_t primitive = memory->ReadMemory<uintptr_t>(root_part + Offsets::BasePart::Primitive);
                            if (!primitive) continue;

                            PlayerCache pc;
                            pc.address = player_addr;
                            pc.character = character;
                            pc.root_part_primitive = primitive;
                            pc.humanoid = FindFirstChildByClass(character, "Humanoid");
                            pc.name = GetInstanceName(player_addr);
                            pc.team = memory->ReadMemory<uintptr_t>(player_addr + Offsets::Player::Team);
                            pc.isTeammate = (local_team != 0 && pc.team == local_team);
                            player_entities.push_back(pc);
                        }
                    }
                }
            }
        }
        last_entity_refresh = now;
    }

    // Fast View Matrix and Camera Refresh
    uintptr_t visual_engine = memory->ReadMemory<uintptr_t>(game_base + Offsets::VisualEngine::Pointer);
    if (visual_engine) {
        cached_view_matrix = memory->ReadMemory<Matrix4x4>(visual_engine + Offsets::VisualEngine::ViewMatrix);
    }
    
    uintptr_t workspace = GetWorkspace();
    if (workspace) {
        uintptr_t camera = memory->ReadMemory<uintptr_t>(workspace + Offsets::Workspace::CurrentCamera);
        if (camera) cached_camera_pos = memory->ReadMemory<Vector3>(camera + Offsets::Camera::Position);
    }

    cached_players.clear();
    for (auto& ent : player_entities)
    {
        Player p;
        p.address = ent.address;
        p.characterAddress = ent.character;
        p.position = memory->ReadMemory<Vector3>(ent.root_part_primitive + Offsets::BasePart::Position);
        p.name = ent.name;
        p.isTeammate = ent.isTeammate;
        
        if (ent.humanoid)
        {
            p.health = memory->ReadMemory<float>(ent.humanoid + Offsets::Humanoid::Health);
            p.maxHealth = memory->ReadMemory<float>(ent.humanoid + Offsets::Humanoid::MaxHealth);
        }
        else { p.health = 100.0f; p.maxHealth = 100.0f; }

        if (p.health > 0)
        {
            p.valid = true;
            cached_players.push_back(p);
        }
    }
}

const std::vector<Player>& ActorLoopClass::GetPlayers() { return cached_players; }
const Matrix4x4& ActorLoopClass::GetCachedViewMatrix() { return cached_view_matrix; }

void ActorLoopClass::Render()
{
    if (!memory) return;
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    ImVec2 display_size = ImGui::GetIO().DisplaySize;
    float sw = display_size.x;
    float sh = display_size.y;

    if (!showMenu) {
        char buf[128];
        sprintf(buf, "noc-external | FPS: %.1f | Players: %d", ImGui::GetIO().Framerate, (int)cached_players.size());
        draw_list->AddText(ImVec2(20, 20), IM_COL32(255, 255, 255, 255), buf);
    }

    if (aimbot.settings.enableAimbot && aimbot.settings.showFOVCircle) {
        draw_list->AddCircle(ImVec2(sw / 2.0f, sh / 2.0f), aimbot.settings.fov, ColorToImU32(aimbot.settings.fovCircleColor), 64);
    }

    if (!settings.enableESP) return;

    for (const auto& player : cached_players)
    {
        if (!settings.showTeam && player.isTeammate) continue;
        
        float distance = GetDistance(cached_camera_pos, player.position);
        if (distance > settings.maxDistance) continue;

        Vector2D screen_pos;
        if (!WorldToScreen(player.position, screen_pos, cached_view_matrix)) continue;

        // Dynamic 3D Box scaling
        float dist_scale = 100.0f / (distance + 0.1f);
        float height = 500.0f * dist_scale;
        float width = height * 0.6f;
        float x = screen_pos.x - width / 2.0f;
        float y = screen_pos.y - height / 2.0f;

        ImU32 color = player.isTeammate ? ColorToImU32(settings.teamBoxColor) : ColorToImU32(settings.enemyBoxColor);
        
        if (settings.enableBox)
        {
            if (settings.enableCornerBox) DrawCornerBox(draw_list, x, y, x + width, y + height, color, settings.boxThickness);
            else DrawBox(draw_list, x, y, x + width, y + height, color, settings.boxThickness);
        }

        if (settings.enableName)
        {
            float font_size = 13.0f * settings.nameSize;
            float text_w = player.name.length() * (font_size * 0.45f);
            ImVec2 txt_pos(screen_pos.x - (text_w * 0.5f), y - font_size - 2);
            draw_list->AddText(NULL, font_size, txt_pos, player.isTeammate ? ColorToImU32(settings.teamNameColor) : ColorToImU32(settings.enemyNameColor), player.name.c_str());
        }

        if (settings.enableHealthBar)
        {
            float hp_ratio = (player.maxHealth > 0) ? (player.health / player.maxHealth) : 0.0f;
            float hp_pct = clamp_v<float>(hp_ratio, 0.0f, 1.0f);
            float bw = 2.0f;
            float bx = x - 5.0f;
            draw_list->AddRectFilled(ImVec2(bx, y), ImVec2(bx + bw, y + height), IM_COL32(40,40,40,200));
            draw_list->AddRectFilled(ImVec2(bx, y + (height * (1.0f - hp_pct))), ImVec2(bx + bw, y + height), IM_COL32((int)(255 * (1.0f - hp_pct)), (int)(255 * hp_pct), 0, 255));
        }

        if (settings.enableSkeleton) DrawSkeleton(draw_list, player.characterAddress, player.isTeammate ? ColorToImU32(settings.teamSkeletonColor) : ColorToImU32(settings.enemySkeletonColor), settings.skeletonThickness);
    }
}

void ActorLoopClass::UpdateAimbot() { aimbot.Update(this); }
void ActorLoopClass::UpdateMisc() { misc.Update(this); }

bool ActorLoopClass::GetBestTargetPlayer(Player& out_player)
{
    if (cached_players.empty()) return false;
    
    ImVec2 display_size = ImGui::GetIO().DisplaySize;
    float centerX = display_size.x / 2.0f;
    float centerY = display_size.y / 2.0f;
    
    float best_score = FLT_MAX; 
    bool found = false;

    for (const auto& player : cached_players)
    {
        if (!aimbot.settings.aimAtTeam && player.isTeammate) continue;
        
        Vector3 aimPoint = player.position;
        if (aimbot.settings.aimAtHead) aimPoint.y += 2.0f;
        
        Vector2D screenPos;
        if (WorldToScreen(aimPoint, screenPos, cached_view_matrix)) {
            float dx = screenPos.x - centerX;
            float dy = screenPos.y - centerY;
            float pixel_dist = sqrtf(dx * dx + dy * dy);
            
            if (pixel_dist <= aimbot.settings.fov) {
                if (pixel_dist < best_score) {
                    best_score = pixel_dist;
                    out_player = player;
                    found = true;
                }
            }
        }
    }
    return found;
}

bool ActorLoopClass::WorldToScreen(const Vector3& world_pos, Vector2D& screen_pos, const Matrix4x4& view_matrix) 
{
    const float* m = &view_matrix.m[0][0];
    float w = m[12] * world_pos.x + m[13] * world_pos.y + m[14] * world_pos.z + m[15];
    if (w < 0.1f) return false;
    float inv_w = 1.0f / w;
    float sx = (m[0] * world_pos.x + m[1] * world_pos.y + m[2] * world_pos.z + m[3]) * inv_w;
    float sy = (m[4] * world_pos.x + m[5] * world_pos.y + m[6] * world_pos.z + m[7]) * inv_w;
    
    ImVec2 display_size = ImGui::GetIO().DisplaySize;
    screen_pos.x = (display_size.x / 2.0f) * (sx + 1.0f);
    screen_pos.y = (display_size.y / 2.0f) * (1.0f - sy);
    return true;
}

Vector3 ActorLoopClass::GetCameraPosition() { return cached_camera_pos; }

Vector3 ActorLoopClass::GetCameraRotation()
{
    uintptr_t workspace = GetWorkspace();
    if (!workspace) return Vector3{};
    uintptr_t camera = memory->ReadMemory<uintptr_t>(workspace + Offsets::Workspace::CurrentCamera);
    Vector3 back_vector = memory->ReadMemory<Vector3>(camera + Offsets::Camera::Rotation + 24);
    Vector3 forward = { -back_vector.x, -back_vector.y, -back_vector.z };
    return { asinf(forward.y) * 57.29577f, atan2f(forward.x, -forward.z) * 57.29577f, 0.0f };
}

uintptr_t ActorLoopClass::GetWorkspace()
{
    if (cached_workspace && std::chrono::high_resolution_clock::now() - last_core_refresh < std::chrono::seconds(10)) return cached_workspace;

    uintptr_t datamodel = GetDataModel();
    if (!datamodel) return 0;
    uintptr_t workspace = memory->ReadMemory<uintptr_t>(datamodel + Offsets::DataModel::Workspace);
    if (!workspace) workspace = FindFirstChildByClass(datamodel, "Workspace");
    
    if (workspace) cached_workspace = workspace;
    return workspace;
}

uintptr_t ActorLoopClass::GetLocalHumanoid()
{
    if (!local_player) return 0;
    uintptr_t character = memory->ReadMemory<uintptr_t>(local_player + Offsets::Player::ModelInstance);
    if (!character) return 0;
    return FindFirstChildByClass(character, "Humanoid");
}

uintptr_t ActorLoopClass::GetLocalPlayerTeam() { 
    if (!local_player) return 0;
    return memory->ReadMemory<uintptr_t>(local_player + Offsets::Player::Team); 
}

float ActorLoopClass::GetDistance(const Vector3& p1, const Vector3& p2) { return sqrtf(powf(p1.x - p2.x, 2) + powf(p1.y - p2.y, 2) + powf(p1.z - p2.z, 2)); }

Vector3 ActorLoopClass::CalculateAngle(const Vector3& from, const Vector3& to)
{
    Vector3 delta = { to.x - from.x, to.y - from.y, to.z - from.z };
    float hyp = sqrtf(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
    return { asinf(delta.y / hyp) * 57.29577f, atan2f(delta.x, -delta.z) * 57.29577f, 0.0f };
}

float ActorLoopClass::GetFOV(const Vector3& target_pos)
{
    Vector3 ang = CalculateAngle(cached_camera_pos, target_pos);
    Vector3 cur = GetCameraRotation();
    float dx = std::remainderf(ang.x - cur.x, 360.0f);
    float dy = std::remainderf(ang.y - cur.y, 360.0f);
    return sqrtf(dx * dx + dy * dy);
}

void ActorLoopClass::Render(Overlay* o) { Render(); }
void ActorLoopClass::SetCameraRotation(const Vector3& r) { }

// Drawing Methods implementation
void ActorLoopClass::DrawCornerBox(ImDrawList* draw_list, float min_x, float min_y, float max_x, float max_y, ImU32 color, float thickness)
{
    float width = max_x - min_x;
    float height = max_y - min_y;
    float corner_length = width * 0.2f;
    
    draw_list->AddLine(ImVec2(min_x, min_y), ImVec2(min_x + corner_length, min_y), color, thickness);
    draw_list->AddLine(ImVec2(min_x, min_y), ImVec2(min_x, min_y + corner_length), color, thickness);
    draw_list->AddLine(ImVec2(max_x - corner_length, min_y), ImVec2(max_x, min_y), color, thickness);
    draw_list->AddLine(ImVec2(max_x, min_y), ImVec2(max_x, min_y + corner_length), color, thickness);
    draw_list->AddLine(ImVec2(min_x, max_y - corner_length), ImVec2(min_x, max_y), color, thickness);
    draw_list->AddLine(ImVec2(min_x, max_y), ImVec2(min_x + corner_length, max_y), color, thickness);
    draw_list->AddLine(ImVec2(max_x - corner_length, max_y), ImVec2(max_x, max_y), color, thickness);
    draw_list->AddLine(ImVec2(max_x, max_y), ImVec2(max_x, max_y - corner_length), color, thickness);
}

void ActorLoopClass::DrawBox(ImDrawList* draw_list, float min_x, float min_y, float max_x, float max_y, ImU32 color, float thickness)
{
    draw_list->AddRect(ImVec2(min_x, min_y), ImVec2(max_x, max_y), color, 0.0f, 0, thickness);
}

void ActorLoopClass::DrawSnapline(ImDrawList* draw_list, float x, float y, ImU32 color, float thickness, int type)
{
    ImVec2 screen_size = ImGui::GetIO().DisplaySize;
    ImVec2 start_pos = ImVec2(screen_size.x * 0.5f, screen_size.y);
    if (type == 1) start_pos.y = 0;
    else if (type == 2) start_pos.y = screen_size.y * 0.5f;
    draw_list->AddLine(start_pos, ImVec2(x, y), color, thickness);
}

void ActorLoopClass::DrawFOVCircle(ImDrawList* draw_list, float fov, ImU32 color)
{
    ImVec2 center = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
    draw_list->AddCircle(center, fov, color, 64, 1.0f);
}

Vector3 ActorLoopClass::GetBonePosition(uintptr_t character, const std::string& bone_name)
{
    if (bone_pos_cache.count(character) && bone_pos_cache[character].count(bone_name))
        return bone_pos_cache[character][bone_name];

    auto& char_bones = bone_cache[character];
    uintptr_t bone = 0;
    if (char_bones.count(bone_name)) bone = char_bones[bone_name];
    else {
        bone = FindFirstChild(character, bone_name);
        if (bone) char_bones[bone_name] = bone;
    }

    if (!bone) return { 0, 0, 0 };
    uintptr_t primitive = memory->ReadMemory<uintptr_t>(bone + Offsets::BasePart::Primitive);
    if (!primitive) return { 0, 0, 0 };
    
    Vector3 pos = memory->ReadMemory<Vector3>(primitive + Offsets::BasePart::Position);
    bone_pos_cache[character][bone_name] = pos;
    return pos;
}

void ActorLoopClass::DrawSkeleton(ImDrawList* draw_list, uintptr_t character, ImU32 color, float thickness)
{
    static const char* bones[] = {"Head", "UpperTorso", "LowerTorso", "LeftUpperArm", "LeftLowerArm", "RightUpperArm", "RightLowerArm", "LeftUpperLeg", "LeftLowerLeg", "RightUpperLeg", "RightLowerLeg"};
    static const std::pair<int, int> connections[] = {{0,1}, {1,2}, {1,3}, {3,4}, {1,5}, {5,6}, {2,7}, {7,8}, {2,9}, {9,10}};
    
    Vector2D p_screen[11];
    bool valid[11] = {false};

    for(int i = 0; i < 11; i++) {
        Vector3 p3d = GetBonePosition(character, bones[i]);
        if (p3d.x != 0) valid[i] = WorldToScreen(p3d, p_screen[i], cached_view_matrix);
    }

    for(const auto& conn : connections) {
        if (valid[conn.first] && valid[conn.second])
            draw_list->AddLine(ImVec2(p_screen[conn.first].x, p_screen[conn.first].y), ImVec2(p_screen[conn.second].x, p_screen[conn.second].y), color, thickness);
    }
}
