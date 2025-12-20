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


static ImU32 ColorToImU32(float* color)
{
    return IM_COL32((int)(color[0] * 255.0f), (int)(color[1] * 255.0f), (int)(color[2] * 255.0f), (int)(color[3] * 255.0f));
}

static std::vector<Player> cached_players;
static std::chrono::high_resolution_clock::time_point last_player_update;
static std::string temp_string;

ActorLoopClass::ActorLoopClass() 
{
    memory = std::make_unique<Memory>();
    game_base = 0;
    local_player = 0;
    
    cached_players.reserve(16);
}

bool ActorLoopClass::Initialize() 
{
    if (!memory->AttachToProcess("RobloxPlayerBeta.exe"))
    {
        std::cout << "Failed to attach to Roblox process" << std::endl;
        return false;
    }
   
    game_base = memory->GetBaseAddress();
    if (!game_base) 
    {
        std::cout << "Failed to get game base address" << std::endl;
        return false;
    }
    
    return true;
}

std::string ActorLoopClass::ReadString(uintptr_t address)
{
    temp_string.clear();
    temp_string.reserve(64);
    
    char character = 0;
    int offset = 0;

    while (offset < 200)
    {
        character = memory->ReadMemory<char>(address + offset);

        if (character == 0)
            break;

        offset++;
        temp_string.push_back(character);
    }

    return temp_string;
}

std::string ActorLoopClass::LengthReadString(uintptr_t string)
{
    const auto length = memory->ReadMemory<int>(string + Offsets::Misc::StringLength);

    if (length >= 16u)
    {
        const auto _new = memory->ReadMemory<uintptr_t>(string);
        return ReadString(_new);
    }
    else
    {
        return ReadString(string);
    }
}

std::string ActorLoopClass::GetInstanceName(uintptr_t instance_address)
{
    const auto _get = memory->ReadMemory<uintptr_t>(instance_address + Offsets::Instance::Name);

    if (_get)
        return LengthReadString(_get);

    return "???";
}

std::string ActorLoopClass::GetInstanceClassName(uintptr_t instance_address)
{
    const auto ptr = memory->ReadMemory<uintptr_t>(instance_address + Offsets::Instance::ClassDescriptor);
    const auto ptr2 = memory->ReadMemory<uintptr_t>(ptr + Offsets::Instance::ClassName);

    if (ptr2)
        return ReadString(ptr2);

    return "???";
}

uintptr_t ActorLoopClass::FindFirstChild(uintptr_t instance_address, const std::string& child_name)
{
    if (!instance_address)
        return 0;

    static std::unordered_map<uintptr_t, std::vector<std::pair<uintptr_t, std::string>>> cache;
    static std::unordered_map<uintptr_t, std::chrono::high_resolution_clock::time_point> last_update;

    auto now = std::chrono::high_resolution_clock::now();
    auto& children = cache[instance_address];
    auto& update_time = last_update[instance_address];

    if (children.empty() || now - update_time > std::chrono::seconds(1))
    {
        children.clear();
        
        auto start = memory->ReadMemory<uintptr_t>(instance_address + Offsets::Instance::ChildrenStart);
        if (!start)
            return 0;

        auto end = memory->ReadMemory<uintptr_t>(start + Offsets::Instance::ChildrenEnd);
        auto childArray = memory->ReadMemory<uintptr_t>(start);
        if (!childArray || childArray >= end)
            return 0;

        children.reserve(32);

        for (uintptr_t current = childArray; current < end; current += 16)
        {
            auto child_instance = memory->ReadMemory<uintptr_t>(current);
            if (!child_instance)
                continue;
            std::string name = GetInstanceName(child_instance);
            children.emplace_back(child_instance, std::move(name));
        }
        update_time = now;
    }

    for (const auto& [child_instance, name] : children)
    {
        if (name == child_name)
            return child_instance;
    }

    return 0;
}

uintptr_t ActorLoopClass::FindFirstChildByClass(uintptr_t instance_address, const std::string& class_name)
{
    if (!instance_address)
        return 0;

    static std::unordered_map<uintptr_t, std::vector<std::pair<uintptr_t, std::string>>> cache;
    static std::unordered_map<uintptr_t, std::chrono::high_resolution_clock::time_point> last_update;

    auto now = std::chrono::high_resolution_clock::now();
    auto& children = cache[instance_address];
    auto& update_time = last_update[instance_address];

    if (children.empty() || now - update_time > std::chrono::seconds(1))
    {
        children.clear();
        children.reserve(32);
        
        auto start = memory->ReadMemory<uintptr_t>(instance_address + Offsets::Instance::ChildrenStart);
        if (!start)
            return 0;

        auto end = memory->ReadMemory<uintptr_t>(start + Offsets::Instance::ChildrenEnd);
        auto childArray = memory->ReadMemory<uintptr_t>(start);
        if (!childArray || childArray >= end)
            return 0;

        for (uintptr_t current = childArray; current < end; current += 16)
        {
            auto child_instance = memory->ReadMemory<uintptr_t>(current);
            if (!child_instance)
                continue;
            std::string classname = GetInstanceClassName(child_instance);
            children.emplace_back(child_instance, std::move(classname));
        }
        update_time = now;
    }

    for (const auto& [child_instance, classname] : children)
    {
        if (classname == class_name)
            return child_instance;
    }

    return 0;
}

std::vector<Player> ActorLoopClass::GetPlayers()
{
    auto now = std::chrono::high_resolution_clock::now();
    
    static std::vector<uintptr_t> cached_player_addresses;
    static std::chrono::high_resolution_clock::time_point last_structure_update;
    
    bool need_structure_update = cached_player_addresses.empty() || 
                                now - last_structure_update > std::chrono::seconds(2);
    
    if (need_structure_update)
    {
        cached_player_addresses.clear();
        
        uintptr_t fake_datamodel = memory->ReadMemory<uintptr_t>(game_base + Offsets::FakeDataModel::Pointer);
        if (!fake_datamodel) return cached_players;
        
        uintptr_t datamodel = memory->ReadMemory<uintptr_t>(fake_datamodel + Offsets::FakeDataModel::RealDataModel);
        if (!datamodel) return cached_players;
        
        uintptr_t players_service = FindFirstChildByClass(datamodel, "Players");
        if (!players_service) return cached_players;
        
        local_player = memory->ReadMemory<uintptr_t>(players_service + Offsets::Player::LocalPlayer);
        
        auto start = memory->ReadMemory<uintptr_t>(players_service + Offsets::Instance::ChildrenStart);
        if (!start) return cached_players;
        auto end = memory->ReadMemory<uintptr_t>(start + Offsets::Instance::ChildrenEnd);
        
        static std::vector<uintptr_t> all_children;
        all_children.clear();
        all_children.reserve(16);
        
        for (auto instances = memory->ReadMemory<uintptr_t>(start); instances != end; instances += 16)
        {
            uintptr_t child = memory->ReadMemory<uintptr_t>(instances);
            if (child) all_children.push_back(child);
        }
        
        for (const auto& child : all_children)
        {
            std::string class_name = GetInstanceClassName(child);
            if (class_name == "Player" && child != local_player)
            {
                cached_player_addresses.push_back(child);
            }
        }
        
        last_structure_update = now;
    }
    
    cached_players.clear();
    cached_players.reserve(cached_player_addresses.size());
   
    uintptr_t local_team = GetLocalPlayerTeam();
    
    for (const auto& player_addr : cached_player_addresses)
    {
        Player player;
        player.address = player_addr;
        player.valid = false;
        player.isTeammate = false;
        player.teamAddress = 0;
        
        player.name = GetInstanceName(player_addr);
        
      
        player.teamAddress = memory->ReadMemory<uintptr_t>(player_addr + Offsets::Player::Team);
        
      
        if (local_team != 0 && player.teamAddress == local_team)
        {
            
            player.isTeammate = true;
        }
        else
        {
          
            player.isTeammate = false;
        }
        
        uintptr_t character = memory->ReadMemory<uintptr_t>(player_addr + Offsets::Player::ModelInstance);
        if (!character) continue;
        player.characterAddress = character;
        
        uintptr_t root_part = FindFirstChild(character, "HumanoidRootPart");
        if (!root_part) continue;
        
        uintptr_t primitive = memory->ReadMemory<uintptr_t>(root_part + Offsets::BasePart::Primitive);
        if (!primitive) continue;
        
        player.position = memory->ReadMemory<Vector3>(primitive + Offsets::BasePart::Position);
        player.size = memory->ReadMemory<Vector3>(root_part + Offsets::BasePart::Size);
        
     
        if (player.size.x <= 0.0f || player.size.y <= 0.0f || player.size.z <= 0.0f)
        {
            player.size = { 2.0f, 5.0f, 1.0f };
        }
        
        player.valid = true;
        
        cached_players.push_back(std::move(player));
    }
    
    return cached_players;
}



void ActorLoopClass::Render()
{
    if (!memory) return;
    
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    
  
    if (!showMenu)
    {
        draw_list->AddText(ImVec2(20, 20), IM_COL32(255, 255, 255, 255), "noc-external | Press Insert for menu");
    }
    
    if (aimbot.settings.enableAimbot)
    {
        ImU32 fov_color = ColorToImU32(aimbot.settings.fovCircleColor);
        DrawFOVCircle(draw_list, aimbot.settings.fov, fov_color);
    }
    
    if (!settings.enableESP) return;
    
    auto view_matrix = GetViewMatrix();
    auto players = GetPlayers();
    
    for (const auto& player : players)
    {
        if (!player.valid) continue;
        
     
        if (!settings.showTeam && player.isTeammate) continue;
        
        
        Vector3 half_size = { player.size.x * 0.5f, player.size.y * 0.5f, player.size.z * 0.5f };
        
       
        Vector3 top_center = { player.position.x, player.position.y + half_size.y, player.position.z };
        Vector3 bottom_center = { player.position.x, player.position.y - half_size.y, player.position.z };
        Vector3 mid_corners[4] = {
            { player.position.x - half_size.x, player.position.y, player.position.z - half_size.z },
            { player.position.x + half_size.x, player.position.y, player.position.z - half_size.z },
            { player.position.x + half_size.x, player.position.y, player.position.z + half_size.z },
            { player.position.x - half_size.x, player.position.y, player.position.z + half_size.z }
        };
        
        
        Vector2D screen_top, screen_bottom;
        Vector2D screen_mid[4];
        
        if (!WorldToScreen(top_center, screen_top, view_matrix) || 
            !WorldToScreen(bottom_center, screen_bottom, view_matrix))
            continue;
        

        float min_x = (screen_top.x < screen_bottom.x) ? screen_top.x : screen_bottom.x;
        float max_x = (screen_top.x > screen_bottom.x) ? screen_top.x : screen_bottom.x;
        float min_y = (screen_top.y < screen_bottom.y) ? screen_top.y : screen_bottom.y;
        float max_y = (screen_top.y > screen_bottom.y) ? screen_top.y : screen_bottom.y;
        

        for (int i = 0; i < 4; i++)
        {
            if (WorldToScreen(mid_corners[i], screen_mid[i], view_matrix))
            {
                if (screen_mid[i].x < min_x) min_x = screen_mid[i].x;
                if (screen_mid[i].x > max_x) max_x = screen_mid[i].x;
                if (screen_mid[i].y < min_y) min_y = screen_mid[i].y;
                if (screen_mid[i].y > max_y) max_y = screen_mid[i].y;
            }
        }
        
       
        float padding = 2.0f;
        min_x -= padding;
        max_x += padding;
        min_y -= padding;
        max_y += padding;
        

        Vector3 camera_pos = GetCameraPosition();
        float distance = GetDistance(camera_pos, player.position);
        
      
        if (distance > settings.maxDistance) continue;
        

        ImU32 box_color, name_color, snapline_color, distance_color;
        if (player.isTeammate)
        {
            box_color = ColorToImU32(settings.teamBoxColor);
            name_color = ColorToImU32(settings.teamNameColor);
            snapline_color = ColorToImU32(settings.teamSnaplineColor);
            distance_color = ColorToImU32(settings.teamDistanceColor);
        }
        else
        {
            box_color = ColorToImU32(settings.enemyBoxColor);
            name_color = ColorToImU32(settings.enemyNameColor);
            snapline_color = ColorToImU32(settings.enemySnaplineColor);
            distance_color = ColorToImU32(settings.enemyDistanceColor);
        }
        

        if (settings.enableSnapline)
        {
            float line_y = (settings.snaplineType == 0) ? max_y : 
                          (settings.snaplineType == 1) ? min_y : 
                          (min_y + max_y) * 0.5f;
            DrawSnapline(draw_list, (min_x + max_x) * 0.5f, line_y, snapline_color, settings.snaplineThickness, settings.snaplineType);
        }
        

        if (settings.enableBox)
        {
            if (settings.enableCornerBox)
                DrawCornerBox(draw_list, min_x, min_y, max_x, max_y, box_color, settings.boxThickness);
            else
                DrawBox(draw_list, min_x, min_y, max_x, max_y, box_color, settings.boxThickness);
        }
        

        float name_offset = 0.0f;
        if (settings.enableName)
        {
            float text_width = player.name.length() * 6.5f * settings.nameSize;
            ImVec2 name_pos(min_x + (max_x - min_x) * 0.5f - text_width * 0.5f, min_y - 20);
            
            ImU32 outline_color_text = IM_COL32(0, 0, 0, 255);
            for (int i = -1; i <= 1; i++)
            {
                for (int j = -1; j <= 1; j++)
                {
                    if (i != 0 || j != 0)
                    {
                        draw_list->AddText(ImVec2(name_pos.x + i, name_pos.y + j), outline_color_text, player.name.c_str());
                    }
                }
            }
            draw_list->AddText(name_pos, name_color, player.name.c_str());
            name_offset = 18.0f;
        }
        
        if (settings.enableDistance)
        {
            char distance_text[32];
            snprintf(distance_text, sizeof(distance_text), "%.0fm", distance);
            float text_width = strlen(distance_text) * 6.5f * settings.nameSize;
            ImVec2 dist_pos(min_x + (max_x - min_x) * 0.5f - text_width * 0.5f, min_y - 20 + name_offset);
            ImU32 outline_color_text = IM_COL32(0, 0, 0, 255);
            for (int i = -1; i <= 1; i++)
            {
                for (int j = -1; j <= 1; j++)
                {
                    if (i != 0 || j != 0)
                    {
                        draw_list->AddText(ImVec2(dist_pos.x + i, dist_pos.y + j), outline_color_text, distance_text);
                    }
                }
            }
            draw_list->AddText(dist_pos, distance_color, distance_text);
            name_offset += 18.0f;
        }
        

        if (settings.enableHealthBar)
        {
            uintptr_t character = memory->ReadMemory<uintptr_t>(player.address + Offsets::Player::ModelInstance);
            if (character)
            {
                uintptr_t humanoid = FindFirstChild(character, "Humanoid");
                if (humanoid)
                {
                    float health = memory->ReadMemory<float>(humanoid + Offsets::Humanoid::Health);
                    float maxHealth = memory->ReadMemory<float>(humanoid + Offsets::Humanoid::MaxHealth);
                    if (maxHealth > 0.0f)
                    {
                        float healthPercent = health / maxHealth;
                        healthPercent = (healthPercent < 0.0f) ? 0.0f : (healthPercent > 1.0f) ? 1.0f : healthPercent;
                        
                        float bar_width = (max_x - min_x) * 0.6f;
                        float bar_height = 4.0f; 
                        float bar_x = min_x + (max_x - min_x) * 0.5f - bar_width * 0.5f;
                        float bar_y = max_y + 5.0f;
                        

                        draw_list->AddRectFilled(ImVec2(bar_x, bar_y), ImVec2(bar_x + bar_width, bar_y + bar_height), IM_COL32(20, 20, 20, 255));
                        
                        ImU32 health_color = IM_COL32(
                            (int)(255 * (1.0f - healthPercent)),
                            (int)(255 * healthPercent),
                            0,
                            255
                        );
                        draw_list->AddRectFilled(ImVec2(bar_x, bar_y), ImVec2(bar_x + bar_width * healthPercent, bar_y + bar_height), health_color);
                        
                        draw_list->AddRect(ImVec2(bar_x - 1, bar_y - 1), ImVec2(bar_x + bar_width + 1, bar_y + bar_height + 1), IM_COL32(0, 0, 0, 255), 0.0f, 0, 1.5f);
                    }
                }
            }
        }
        

        if (settings.enableSkeleton && player.characterAddress)
        {
            ImU32 skeleton_color = player.isTeammate ? 
                ColorToImU32(settings.teamSkeletonColor) : 
                ColorToImU32(settings.enemySkeletonColor);
                
            DrawSkeleton(draw_list, player.characterAddress, skeleton_color, settings.skeletonThickness);
        }
    }
    
}

void ActorLoopClass::Render(Overlay* overlay)
{
    Render();
}




Matrix4x4 ActorLoopClass::GetViewMatrix()
{
    Matrix4x4 view_matrix{};
    
    uintptr_t visual_engine = memory->ReadMemory<uintptr_t>(game_base + Offsets::VisualEngine::Pointer);
    if (!visual_engine) return view_matrix;
    
    view_matrix = memory->ReadMemory<Matrix4x4>(visual_engine + Offsets::VisualEngine::ViewMatrix);
    return view_matrix;
}

bool ActorLoopClass::WorldToScreen(const Vector3& world_pos, Vector2D& screen_pos, const Matrix4x4& view_matrix) 
{
    const float* m = &view_matrix.m[0][0];

    _mm_prefetch((const char*)m, _MM_HINT_T0);

    float w = m[12] * world_pos.x + m[13] * world_pos.y + m[14] * world_pos.z + m[15];

    if (w <= 0.001f)
        return false;

    float inv_w = 1.0f / w;

    screen_pos.x = (m[0] * world_pos.x + m[1] * world_pos.y + m[2] * world_pos.z + m[3]) * inv_w;
    screen_pos.y = (m[4] * world_pos.x + m[5] * world_pos.y + m[6] * world_pos.z + m[7]) * inv_w;

    static constexpr float screen_width_half = 960.0f;
    static constexpr float screen_height_half = 540.0f;

    screen_pos.x = screen_width_half * (screen_pos.x + 1.0f);
    screen_pos.y = screen_height_half * (1.0f - screen_pos.y);

    return true;
}

Vector3 ActorLoopClass::GetCameraPosition()
{
    Vector3 camera_pos{};
    
    uintptr_t fake_datamodel = memory->ReadMemory<uintptr_t>(game_base + Offsets::FakeDataModel::Pointer);
    if (!fake_datamodel) return camera_pos;
    
    uintptr_t datamodel = memory->ReadMemory<uintptr_t>(fake_datamodel + Offsets::FakeDataModel::RealDataModel);
    if (!datamodel) return camera_pos;
    
    uintptr_t workspace = FindFirstChildByClass(datamodel, "Workspace");
    if (!workspace) return camera_pos;
    
    uintptr_t camera = memory->ReadMemory<uintptr_t>(workspace + Offsets::Workspace::CurrentCamera);
    if (!camera) return camera_pos;
    
    camera_pos = memory->ReadMemory<Vector3>(camera + Offsets::Camera::Position);
    return camera_pos;
}

Vector3 ActorLoopClass::GetCameraRotation()
{
    Vector3 camera_rot{};
    
    uintptr_t fake_datamodel = memory->ReadMemory<uintptr_t>(game_base + Offsets::FakeDataModel::Pointer);
    if (!fake_datamodel) return camera_rot;
    
    uintptr_t datamodel = memory->ReadMemory<uintptr_t>(fake_datamodel + Offsets::FakeDataModel::RealDataModel);
    if (!datamodel) return camera_rot;
    
    uintptr_t workspace = FindFirstChildByClass(datamodel, "Workspace");
    if (!workspace) return { 0,0,0 };
    
    uintptr_t camera = memory->ReadMemory<uintptr_t>(workspace + Offsets::Workspace::CurrentCamera);
    if (!camera) return { 0,0,0 };
    
    Vector3 back_vector = memory->ReadMemory<Vector3>(camera + Offsets::Camera::Rotation + 24);
    Vector3 forward = { -back_vector.x, -back_vector.y, -back_vector.z };
    float pitch = asinf(forward.y) * (180.0f / 3.14159265359f);
    float yaw = atan2f(forward.x, -forward.z) * (180.0f / 3.14159265359f);
    
    return { pitch, yaw, 0.0f };
}

void ActorLoopClass::SetCameraRotation(const Vector3& rotation)
{
    Vector3 current_rot = GetCameraRotation();
    
    float delta_pitch = rotation.x - current_rot.x;
    float delta_yaw = rotation.y - current_rot.y;
    while (delta_pitch > 180.0f) delta_pitch -= 360.0f;
    while (delta_pitch < -180.0f) delta_pitch += 360.0f;
    while (delta_yaw > 180.0f) delta_yaw -= 360.0f;
    while (delta_yaw < -180.0f) delta_yaw += 360.0f;
    
    if (abs(delta_pitch) > 0.5f || abs(delta_yaw) > 0.5f)
    {
        float px_per_degree = 14.0f;
        float step_pitch = delta_pitch;
        float step_yaw = delta_yaw;
        if (aimbot.settings.smoothAim && aimbot.settings.smoothness > 0.0f)
        {
            step_pitch /= aimbot.settings.smoothness;
            step_yaw /= aimbot.settings.smoothness;
        }

        LONG move_x = (LONG)(step_yaw * px_per_degree);
        LONG move_y = (LONG)(-step_pitch * px_per_degree);
        if (abs(move_x) > 500) move_x = (move_x > 0) ? 500 : -500;
        if (abs(move_y) > 500) move_y = (move_y > 0) ? 500 : -500;
        INPUT input = { 0 };
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_MOVE;
        input.mi.dx = move_x;
        input.mi.dy = move_y;
        SendInput(1, &input, sizeof(INPUT));
    }
}

float ActorLoopClass::GetDistance(const Vector3& pos1, const Vector3& pos2)
{
    float dx = pos1.x - pos2.x;
    float dy = pos1.y - pos2.y;
    float dz = pos1.z - pos2.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

Vector3 ActorLoopClass::CalculateAngle(const Vector3& from, const Vector3& to)
{
    Vector3 delta;
    delta.x = to.x - from.x;
    delta.y = to.y - from.y;
    delta.z = to.z - from.z;
    
    float hyp = sqrtf(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
    if (hyp < 0.001f) return Vector3{ 0.0f, 0.0f, 0.0f };
    
    float pitch = asinf(delta.y / hyp) * (180.0f / 3.14159265359f);
    float yaw = atan2f(delta.x, -delta.z) * (180.0f / 3.14159265359f);
    
    Vector3 angle;
    angle.x = pitch;
    angle.y = yaw;
    angle.z = 0.0f;
    
    return angle;
}

Vector3 ActorLoopClass::SmoothAngle(const Vector3& current, const Vector3& target, float smoothness)
{
    Vector3 smoothed;
    smoothed.x = current.x + (target.x - current.x) / smoothness;
    smoothed.y = current.y + (target.y - current.y) / smoothness;
    smoothed.z = 0.0f;
    return smoothed;
}

float ActorLoopClass::GetFOV(const Vector3& target_pos)
{
    Vector3 camera_pos = GetCameraPosition();
    Vector3 camera_rot = GetCameraRotation();
    Vector3 angle_to_target = CalculateAngle(camera_pos, target_pos);
    
    float delta_x = angle_to_target.x - camera_rot.x;
    float delta_y = angle_to_target.y - camera_rot.y;
    
    
    while (delta_x > 180.0f) delta_x -= 360.0f;
    while (delta_x < -180.0f) delta_x += 360.0f;
    while (delta_y > 180.0f) delta_y -= 360.0f;
    while (delta_y < -180.0f) delta_y += 360.0f;
    
    return sqrtf(delta_x * delta_x + delta_y * delta_y);
}

Player* ActorLoopClass::GetBestTarget()
{
    if (!memory) return nullptr;
    
    auto players = GetPlayers();
    if (players.empty()) return nullptr;
    
    Vector3 camera_pos = GetCameraPosition();
    Vector3 camera_rot = GetCameraRotation();
    
    Player* best_target = nullptr;
    float best_fov = aimbot.settings.fov;
    float best_distance = aimbot.settings.maxDistance;
    
    for (auto& player : players)
    {
        if (!player.valid || player.address == local_player) continue;
        

        if (!aimbot.settings.aimAtTeam && player.isTeammate) continue;
        
        float distance = GetDistance(camera_pos, player.position);
        if (distance > aimbot.settings.maxDistance) continue;
        
        Vector3 aim_point = player.position;
        if (aimbot.settings.aimAtHead)
        {
            aim_point.y += player.size.y * 0.8f;
        }
        else if (aimbot.settings.aimAtBody)
        {
            aim_point.y += player.size.y * 0.3f;
        }
        
        float fov = GetFOV(aim_point);
        if (fov > aimbot.settings.fov) continue;
        if (aimbot.settings.visibleOnly)
        {
            Vector2D screen_pos;
            Matrix4x4 view_matrix = GetViewMatrix();
            if (!WorldToScreen(aim_point, screen_pos, view_matrix))
                continue;
        }

        if (fov < best_fov || (fov == best_fov && distance < best_distance))
        {
            best_fov = fov;
            best_distance = distance;
            best_target = &player;
        }
    }
    
    return best_target;
}

void ActorLoopClass::UpdateAimbot()
{
    aimbot.Update(this);
}

uintptr_t ActorLoopClass::GetLocalHumanoid()
{
    if (!memory || !local_player) return 0;
    
    uintptr_t character = memory->ReadMemory<uintptr_t>(local_player + Offsets::Player::ModelInstance);
    if (!character) return 0;

    uintptr_t humanoid = FindFirstChild(character, "Humanoid");
    return humanoid;
}

uintptr_t ActorLoopClass::GetWorkspace()
{
    if (!memory || !game_base) return 0;
    
    uintptr_t fake_datamodel = memory->ReadMemory<uintptr_t>(game_base + Offsets::FakeDataModel::Pointer);
    if (!fake_datamodel) return 0;
    
    uintptr_t datamodel = memory->ReadMemory<uintptr_t>(fake_datamodel + Offsets::FakeDataModel::RealDataModel);
    if (!datamodel) return 0;
    
    uintptr_t workspace = FindFirstChildByClass(datamodel, "Workspace");
    return workspace;
}

uintptr_t ActorLoopClass::GetLocalPlayerTeam()
{
    if (!memory || !local_player) return 0;
    
    uintptr_t team = memory->ReadMemory<uintptr_t>(local_player + Offsets::Player::Team);
    return team;
}

bool ActorLoopClass::IsTeammate(uintptr_t player_address, uintptr_t local_team_address)
{
    if (!memory || !player_address) return false;
    
    uintptr_t player_team = memory->ReadMemory<uintptr_t>(player_address + Offsets::Player::Team);
    
    if (local_team_address == 0 && player_team == 0) return false;
    return (local_team_address != 0 && player_team == local_team_address);
}

/*

        case 0: 
        {
            
            colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
            colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.12f, 0.94f);
            colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.08f, 0.10f, 0.94f);
            colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.12f, 0.14f, 0.94f);
            colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
            colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
            colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);
            colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
            colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
            colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.40f, 0.80f, 1.00f);
            colors[ImGuiCol_CheckMark] = ImVec4(0.20f, 0.60f, 1.00f, 1.00f);
            colors[ImGuiCol_SliderGrab] = ImVec4(0.20f, 0.60f, 1.00f, 1.00f);
            colors[ImGuiCol_SliderGrabActive] = ImVec4(0.30f, 0.70f, 1.00f, 1.00f);
            colors[ImGuiCol_Button] = ImVec4(0.20f, 0.40f, 0.80f, 1.00f);
            colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.50f, 0.90f, 1.00f);
            colors[ImGuiCol_ButtonActive] = ImVec4(0.40f, 0.60f, 1.00f, 1.00f);
            colors[ImGuiCol_Header] = ImVec4(0.20f, 0.40f, 0.80f, 0.50f);
            colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.50f, 0.90f, 0.80f);
            colors[ImGuiCol_HeaderActive] = ImVec4(0.40f, 0.60f, 1.00f, 1.00f);
            colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
            colors[ImGuiCol_TabHovered] = ImVec4(0.25f, 0.45f, 0.85f, 1.00f);
            colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.40f, 0.80f, 1.00f);
            break;
        }
        case 1: 
        {
            ImGui::StyleColorsLight();
            break;
        }
        case 2: 
        {
            
            colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
            colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
            colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.05f, 0.12f, 0.95f); 
            colors[ImGuiCol_ChildBg] = ImVec4(0.06f, 0.04f, 0.10f, 0.95f);
            colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.07f, 0.15f, 0.98f);
            colors[ImGuiCol_Border] = ImVec4(0.50f, 0.25f, 0.75f, 0.60f); 
            colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.10f, 0.20f, 1.00f);
            colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.15f, 0.35f, 1.00f);
            colors[ImGuiCol_FrameBgActive] = ImVec4(0.35f, 0.20f, 0.50f, 1.00f);
            colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.06f, 0.15f, 1.00f);
            colors[ImGuiCol_TitleBgActive] = ImVec4(0.60f, 0.30f, 0.85f, 1.00f); 
            colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.08f, 0.05f, 0.12f, 1.00f);
            colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.06f, 0.15f, 1.00f);
            colors[ImGuiCol_ScrollbarBg] = ImVec4(0.08f, 0.05f, 0.12f, 1.00f);
            colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.55f, 0.28f, 0.80f, 1.00f);
            colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.65f, 0.35f, 0.90f, 1.00f);
            colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.75f, 0.40f, 1.00f, 1.00f);
            colors[ImGuiCol_CheckMark] = ImVec4(0.70f, 0.35f, 0.95f, 1.00f);
            colors[ImGuiCol_SliderGrab] = ImVec4(0.60f, 0.30f, 0.85f, 1.00f);
            colors[ImGuiCol_SliderGrabActive] = ImVec4(0.75f, 0.40f, 1.00f, 1.00f);
            colors[ImGuiCol_Button] = ImVec4(0.50f, 0.25f, 0.75f, 1.00f);
            colors[ImGuiCol_ButtonHovered] = ImVec4(0.65f, 0.35f, 0.90f, 1.00f);
            colors[ImGuiCol_ButtonActive] = ImVec4(0.75f, 0.40f, 1.00f, 1.00f);
            colors[ImGuiCol_Header] = ImVec4(0.45f, 0.22f, 0.70f, 0.60f);
            colors[ImGuiCol_HeaderHovered] = ImVec4(0.60f, 0.30f, 0.85f, 0.80f);
            colors[ImGuiCol_HeaderActive] = ImVec4(0.70f, 0.35f, 0.95f, 1.00f);
            colors[ImGuiCol_Separator] = ImVec4(0.50f, 0.25f, 0.75f, 0.50f);
            colors[ImGuiCol_SeparatorHovered] = ImVec4(0.60f, 0.30f, 0.85f, 0.78f);
            colors[ImGuiCol_SeparatorActive] = ImVec4(0.70f, 0.35f, 0.95f, 1.00f);
            colors[ImGuiCol_ResizeGrip] = ImVec4(0.55f, 0.28f, 0.80f, 0.20f);
            colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.65f, 0.35f, 0.90f, 0.67f);
            colors[ImGuiCol_ResizeGripActive] = ImVec4(0.75f, 0.40f, 1.00f, 0.95f);
            colors[ImGuiCol_Tab] = ImVec4(0.12f, 0.08f, 0.18f, 1.00f);
            colors[ImGuiCol_TabHovered] = ImVec4(0.60f, 0.30f, 0.85f, 0.80f);
            colors[ImGuiCol_TabActive] = ImVec4(0.60f, 0.30f, 0.85f, 1.00f);
            colors[ImGuiCol_TabUnfocused] = ImVec4(0.10f, 0.06f, 0.15f, 1.00f);
            colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.15f, 0.10f, 0.20f, 1.00f);
            colors[ImGuiCol_PlotLines] = ImVec4(0.70f, 0.35f, 0.95f, 1.00f);
            colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.80f, 0.45f, 1.00f, 1.00f);
            colors[ImGuiCol_PlotHistogram] = ImVec4(0.70f, 0.35f, 0.95f, 1.00f);
            colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.80f, 0.45f, 1.00f, 1.00f);
            colors[ImGuiCol_TableHeaderBg] = ImVec4(0.12f, 0.08f, 0.18f, 1.00f);
            colors[ImGuiCol_TableBorderStrong] = ImVec4(0.50f, 0.25f, 0.75f, 1.00f);
            colors[ImGuiCol_TableBorderLight] = ImVec4(0.40f, 0.20f, 0.60f, 1.00f);
            colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.10f, 0.06f, 0.15f, 0.50f);
            colors[ImGuiCol_TextSelectedBg] = ImVec4(0.60f, 0.30f, 0.85f, 0.35f);
            colors[ImGuiCol_DragDropTarget] = ImVec4(0.70f, 0.35f, 0.95f, 0.90f);
            colors[ImGuiCol_NavHighlight] = ImVec4(0.70f, 0.35f, 0.95f, 1.00f);
            colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.70f, 0.35f, 0.95f, 0.70f);
            colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
            colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
*/

void ActorLoopClass::UpdateMisc()
{
    misc.Update(this);
}

