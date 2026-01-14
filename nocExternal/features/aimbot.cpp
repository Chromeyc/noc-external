#include "aimbot.h"
#include "esp.h"
#include "../sdk/offsets.h"
#include "../imgui/imgui.h"
#include <cmath>
#include <algorithm>

Aimbot::Aimbot() : currentTargetFound(false), lockedTargetAddress(0) {}

void Aimbot::MoveMouse(const Vector2D& targetPos) {
    float screenW = (float)GetSystemMetrics(SM_CXSCREEN);
    float screenH = (float)GetSystemMetrics(SM_CYSCREEN);
    
    float centerX = screenW * 0.5f;
    float centerY = screenH * 0.5f;
    
    float dx = targetPos.x - centerX;
    float dy = targetPos.y - centerY;

    float final_dx = 0;
    float final_dy = 0;

    if (settings.smoothAim) {
        float smooth = settings.smoothness;
        if (smooth < 1.0f) smooth = 1.0f;
        
        final_dx = dx / smooth;
        final_dy = dy / smooth;

        if (std::abs(final_dx) < 0.5f && std::abs(dx) > 0.0f) final_dx = (dx > 0) ? 1.0f : -1.0f;
        if (std::abs(final_dy) < 0.5f && std::abs(dy) > 0.0f) final_dy = (dy > 0) ? 1.0f : -1.0f;
        
        if (std::abs(dx) < 2.0f && std::abs(dy) < 2.0f) return;
    } else {
        final_dx = dx;
        final_dy = dy;
    }

    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    input.mi.dx = (LONG)final_dx;
    input.mi.dy = (LONG)final_dy;
    SendInput(1, &input, sizeof(INPUT));
}

void Aimbot::Update(ActorLoopClass* actorLoop) {
    if (!settings.enableAimbot) {
        lockedTargetAddress = 0;
        currentTargetFound = false;
        return;
    }
    
    if ((GetAsyncKeyState(settings.aimKey) & 0x8000) == 0) {
        lockedTargetAddress = 0;
        currentTargetFound = false;
        return;
    }

    Player target;
    bool targetValid = false;

    // 1. Sticky Target: Check if we are already locked onto someone
    if (lockedTargetAddress != 0) {
        const auto& players = actorLoop->GetPlayers();
        for (const auto& p : players) {
            if (p.address == lockedTargetAddress && p.health > 0) {
                target = p;
                targetValid = true;
                break;
            }
        }
        
        // If they died or left, clear the lock so we can find a new one
        if (!targetValid) lockedTargetAddress = 0;
    }

    // 2. Initial Target: If no one is locked, find the best one in FOV
    if (lockedTargetAddress == 0) {
        if (actorLoop->GetBestTargetPlayer(target)) {
            lockedTargetAddress = target.address;
            targetValid = true;
        }
    }

    currentTargetFound = targetValid;
    if (!targetValid) return;
    
    Vector3 aimPoint = target.position;
    if (settings.aimAtHead) {
        aimPoint.y += 2.2f;
    } 

    Vector2D screenTarget;
    if (actorLoop->WorldToScreen(aimPoint, screenTarget, actorLoop->GetCachedViewMatrix())) {
        MoveMouse(screenTarget);
    }
}
