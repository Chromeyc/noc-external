#pragma once
#include <vector>
#include <string>
#include <Windows.h>
#include "../sdk/sdk.h"


class ActorLoopClass;
struct Player;

struct AimbotSettings {
    bool enableAimbot = false;
    bool aimAtHead = true;
    bool smoothAim = true;
    float fov = 100.0f;
    float smoothness = 5.0f;
    int aimKey = VK_RBUTTON;
    float maxDistance = 500.0f;
    bool showFOVCircle = true;
    bool enablePrediction = false;
    float predictionFactor = 1.5f;
    float fovCircleColor[4] = { 0.0f, 0.831f, 1.0f, 0.8f };
    
    bool visibleOnly = true;
    bool aimAtTeam = false;
    bool aimAtBody = false;
};

class Aimbot {
public:
    Aimbot();
    ~Aimbot() = default;

    void Update(ActorLoopClass* actorLoop);
    void RenderUI();

    AimbotSettings settings;

private:
    Player* GetBestTarget(ActorLoopClass* actorLoop);
    void MoveMouse(const Vector2D& screenPos);
    
    Player* currentTarget = nullptr;
};
