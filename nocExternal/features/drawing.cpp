#include "esp.h"
#include "../imgui/imgui.h"
#include "../sdk/offsets.h"


static ImU32 ColorToImU32(float* color)
{
    return ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], color[3]));
}

void ActorLoopClass::DrawCornerBox(ImDrawList* draw_list, float min_x, float min_y, float max_x, float max_y, ImU32 color, float thickness)
{
    float width = max_x - min_x;
    float height = max_y - min_y;
    float corner_length = (width * 0.25f < height * 0.25f) ? width * 0.25f : height * 0.25f;
    
    
    if (settings.enableBoxOutline)
    {
        ImU32 outline_color = IM_COL32(0, 0, 0, 255);
        float outline_thickness = settings.boxOutlineThickness;
        
        draw_list->AddLine(ImVec2(min_x - 1, min_y + corner_length), ImVec2(min_x - 1, min_y - 1), outline_color, outline_thickness);
        draw_list->AddLine(ImVec2(min_x - 1, min_y - 1), ImVec2(min_x + corner_length, min_y - 1), outline_color, outline_thickness);
        
        draw_list->AddLine(ImVec2(max_x + 1 - corner_length, min_y - 1), ImVec2(max_x + 1, min_y - 1), outline_color, outline_thickness);
        draw_list->AddLine(ImVec2(max_x + 1, min_y - 1), ImVec2(max_x + 1, min_y + corner_length), outline_color, outline_thickness);
        
        draw_list->AddLine(ImVec2(min_x - 1, max_y + 1 - corner_length), ImVec2(min_x - 1, max_y + 1), outline_color, outline_thickness);
        draw_list->AddLine(ImVec2(min_x - 1, max_y + 1), ImVec2(min_x + corner_length, max_y + 1), outline_color, outline_thickness);
        
        draw_list->AddLine(ImVec2(max_x + 1 - corner_length, max_y + 1), ImVec2(max_x + 1, max_y + 1), outline_color, outline_thickness);
        draw_list->AddLine(ImVec2(max_x + 1, max_y + 1), ImVec2(max_x + 1, max_y + 1 - corner_length), outline_color, outline_thickness);
    }
    
    draw_list->AddLine(ImVec2(min_x, min_y + corner_length), ImVec2(min_x, min_y), color, thickness);
    draw_list->AddLine(ImVec2(min_x, min_y), ImVec2(min_x + corner_length, min_y), color, thickness);
    
    draw_list->AddLine(ImVec2(max_x - corner_length, min_y), ImVec2(max_x, min_y), color, thickness);
    draw_list->AddLine(ImVec2(max_x, min_y), ImVec2(max_x, min_y + corner_length), color, thickness);
    
    draw_list->AddLine(ImVec2(min_x, max_y - corner_length), ImVec2(min_x, max_y), color, thickness);
    draw_list->AddLine(ImVec2(min_x, max_y), ImVec2(min_x + corner_length, max_y), color, thickness);
    
    draw_list->AddLine(ImVec2(max_x - corner_length, max_y), ImVec2(max_x, max_y), color, thickness);
    draw_list->AddLine(ImVec2(max_x, max_y), ImVec2(max_x, max_y - corner_length), color, thickness);
}

void ActorLoopClass::DrawBox(ImDrawList* draw_list, float min_x, float min_y, float max_x, float max_y, ImU32 color, float thickness)
{

    if (settings.enableBoxOutline)
    {
        ImU32 outline_color = IM_COL32(0, 0, 0, 255);
        float outline_thickness = settings.boxOutlineThickness + thickness;
        draw_list->AddRect(ImVec2(min_x - 1, min_y - 1), ImVec2(max_x + 1, max_y + 1), outline_color, 4.0f, 0, outline_thickness);
    }
    
    
    draw_list->AddRect(ImVec2(min_x, min_y), ImVec2(max_x, max_y), color, 4.0f, 0, thickness);
}

void ActorLoopClass::DrawSnapline(ImDrawList* draw_list, float x, float y, ImU32 color, float thickness, int type)
{
    ImVec2 screen_size = ImGui::GetIO().DisplaySize;
    float screen_center_x = screen_size.x * 0.5f;
    float screen_bottom_y = screen_size.y;
    
    ImVec2 start_pos, end_pos;
    
    switch (type)
    {
    case 0:
        start_pos = ImVec2(screen_center_x, screen_bottom_y);
        end_pos = ImVec2(x, y);
        break;
    case 1:
        start_pos = ImVec2(screen_center_x, 0.0f);
        end_pos = ImVec2(x, y);
        break;
    case 2:
        start_pos = ImVec2(screen_center_x, screen_size.y * 0.5f);
        end_pos = ImVec2(x, y);
        break;
    default:
        return;
    }
    
    draw_list->PathLineTo(start_pos);
    draw_list->PathLineTo(end_pos);
    draw_list->PathStroke(color, 0, thickness);
}

void ActorLoopClass::DrawFOVCircle(ImDrawList* draw_list, float fov, ImU32 color)
{
    if (!aimbot.settings.showFOVCircle || !aimbot.settings.enableAimbot) return;
    
    ImVec2 screen_size = ImGui::GetIO().DisplaySize;
    ImVec2 center = ImVec2(screen_size.x * 0.5f, screen_size.y * 0.5f);
    
 
    float fov_ratio = fov / 90.0f;
    float radius = 540.0f * fov_ratio * (screen_size.y / 1080.0f);
    
   
    draw_list->AddCircle(center, radius, color, 128, 2.0f);
}

Vector3 ActorLoopClass::GetBonePosition(uintptr_t character, const std::string& bone_name)
{
    uintptr_t bone = FindFirstChild(character, bone_name);
    if (!bone) return { 0, 0, 0 };

    uintptr_t primitive = memory->ReadMemory<uintptr_t>(bone + Offsets::BasePart::Primitive);
    if (!primitive) return { 0, 0, 0 };

    return memory->ReadMemory<Vector3>(primitive + Offsets::BasePart::Position);
}

void ActorLoopClass::DrawSkeleton(ImDrawList* draw_list, uintptr_t character, ImU32 color, float thickness)
{
    if (!character) return;
    
    static const std::vector<std::pair<std::string, std::string>> boneConnections = {
        {"Head", "UpperTorso"},
        {"UpperTorso", "LowerTorso"},
        
        {"UpperTorso", "LeftUpperArm"},
        {"LeftUpperArm", "LeftLowerArm"},
        {"LeftLowerArm", "LeftHand"},
        
        {"UpperTorso", "RightUpperArm"},
        {"RightUpperArm", "RightLowerArm"},
        {"RightLowerArm", "RightHand"},
        
        {"LowerTorso", "LeftUpperLeg"},
        {"LeftUpperLeg", "LeftLowerLeg"},
        {"LeftLowerLeg", "LeftFoot"},
        
        {"LowerTorso", "RightUpperLeg"},
        {"RightUpperLeg", "RightLowerLeg"},
        {"RightLowerLeg", "RightFoot"}
    };

    Matrix4x4 view_matrix = GetViewMatrix();

    for (const auto& connection : boneConnections)
    {
        Vector3 pos1_3d = GetBonePosition(character, connection.first);
        Vector3 pos2_3d = GetBonePosition(character, connection.second);

        if (pos1_3d.x == 0 && pos1_3d.y == 0 && pos1_3d.z == 0) continue;
        if (pos2_3d.x == 0 && pos2_3d.y == 0 && pos2_3d.z == 0) continue;

        Vector2D pos1_2d, pos2_2d;
        if (WorldToScreen(pos1_3d, pos1_2d, view_matrix) && WorldToScreen(pos2_3d, pos2_2d, view_matrix))
        {
            draw_list->AddLine(ImVec2(pos1_2d.x, pos1_2d.y), ImVec2(pos2_2d.x, pos2_2d.y), color, thickness);
        }
    }
}
