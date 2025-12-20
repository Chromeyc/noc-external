#pragma once
#include <chrono>
#include <vector>
#include <string>
#include <Windows.h>
#include "../sdk/sdk.h"


class ActorLoopClass;

struct MiscSettings {
    bool enableSpeed = false;
    bool enableGravity = false;
    bool enableJumpPower = false;
    
    float speed = 16.0f;
    float gravity = 196.2f;
    float jumpPower = 50.0f;
    
    float originalSpeed = 16.0f;
    float originalGravity = 196.2f;
    float originalJumpPower = 50.0f;
    
    bool originalSpeedStored = false;
    bool originalGravityStored = false;
    bool originalJumpPowerStored = false;
    
    float lastWrittenSpeed = -1.0f;
    float lastWrittenGravity = -1.0f;
    float lastWrittenJumpPower = -1.0f;
    
    std::chrono::high_resolution_clock::time_point lastSpeedWrite = std::chrono::high_resolution_clock::time_point();
    std::chrono::high_resolution_clock::time_point lastGravityWrite = std::chrono::high_resolution_clock::time_point();
    std::chrono::high_resolution_clock::time_point lastJumpPowerWrite = std::chrono::high_resolution_clock::time_point();
};

class Misc {
public:
    Misc();
    ~Misc() = default;

    void Update(ActorLoopClass* actorLoop);

    MiscSettings settings;

private:
    void UpdateSpeed(ActorLoopClass* actorLoop);
    void UpdateGravity(ActorLoopClass* actorLoop);
};
