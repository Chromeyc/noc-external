#include "aimbot.h"
#include "esp.h"
#include "../sdk/offsets.h"
#include "../imgui/imgui.h"
#include <cmath>
#include <algorithm>

Aimbot::Aimbot() {
}

Player* Aimbot::GetBestTarget(ActorLoopClass* actorLoop) {
    auto players = actorLoop->GetPlayers();
    if (players.empty()) return nullptr;

    Vector3 cameraPos = actorLoop->GetCameraPosition();
    
    Player* bestTarget = nullptr;
    float bestScore = FLT_MAX;

    Matrix4x4 viewMatrix = actorLoop->GetViewMatrix();
    Vector2D screenCenter = { ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f };

    for (auto& player : players) {
        if (!player.valid || player.address == actorLoop->GetLocalPlayer()) continue;
        if (!settings.aimAtTeam && player.isTeammate) continue;

        float distance = actorLoop->GetDistance(cameraPos, player.position);
        if (distance > settings.maxDistance) continue;
        
        Vector3 aimPos = player.position;
        if (settings.aimAtHead) aimPos.y += player.size.y * 0.5f; 
        
        Vector2D screenPos;
        if (!actorLoop->WorldToScreen(aimPos, screenPos, viewMatrix)) continue;

        float distToCrosshair = sqrt(pow(screenPos.x - screenCenter.x, 2) + pow(screenPos.y - screenCenter.y, 2));
        
        if (distToCrosshair > settings.fov) continue;
        
        if (distToCrosshair < bestScore) {
            bestScore = distToCrosshair;
            bestTarget = &player;
        }
    }
    
    return bestTarget;
}

void Aimbot::MoveMouse(const Vector2D& targetPos) {
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    float centerX = screenSize.x * 0.5f;
    float centerY = screenSize.y * 0.5f;
    
    float dx = targetPos.x - centerX;
    float dy = targetPos.y - centerY;

    if (settings.smoothAim) {
        float smooth = settings.smoothness;
        if (smooth < 1.5f) smooth = 1.5f; 
        
        dx /= smooth;
        dy /= smooth;
    }
    
    if (std::abs(dx) < 2.0f && std::abs(dy) < 2.0f) return;
    
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    input.mi.dx = (LONG)dx;
    input.mi.dy = (LONG)dy;
    SendInput(1, &input, sizeof(INPUT));
}

void Aimbot::Update(ActorLoopClass* actorLoop) {
    if (!settings.enableAimbot) return;
    
    if ((GetAsyncKeyState(settings.aimKey) & 0x8000) == 0) {
        currentTarget = nullptr;
        return;
    }

    Player* target = GetBestTarget(actorLoop);
    if (!target) return;
    
    Vector3 aimPoint = target->position;
    
    if (settings.aimAtHead) {
        aimPoint.y += target->size.y * 0.5f; 
    } else if (settings.aimAtBody) {
    }

    Vector2D screenTarget;
    Matrix4x4 viewMatrix = actorLoop->GetViewMatrix();
    if (actorLoop->WorldToScreen(aimPoint, screenTarget, viewMatrix)) {
        MoveMouse(screenTarget);
    }
}
