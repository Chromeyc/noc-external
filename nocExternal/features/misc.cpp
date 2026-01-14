#include "misc.h"
#include "esp.h"
#include "../sdk/offsets.h"
#include <iostream>
#include <algorithm>
#include <vector>

Misc::Misc() {
}

void Misc::Update(ActorLoopClass* actorLoop) {
    if (!actorLoop) return;
    
    UpdateSpeed(actorLoop);
    UpdateGravity(actorLoop);
}

void Misc::UpdateSpeed(ActorLoopClass* actorLoop) {
    if (!actorLoop->memory) return;

    if (settings.enableSpeed) {
        uintptr_t humanoid = actorLoop->GetLocalHumanoid();
        if (humanoid) {
            float targetSpeed = settings.speed;
            actorLoop->memory->WriteMemory<float>(humanoid + Offsets::Humanoid::Walkspeed, targetSpeed);
            actorLoop->memory->WriteMemory<float>(humanoid + Offsets::Humanoid::WalkspeedCheck, targetSpeed);
            settings.lastWrittenSpeed = targetSpeed;
        }
    } else {
        if (settings.lastWrittenSpeed >= 0.0f) {
            uintptr_t humanoid = actorLoop->GetLocalHumanoid();
            if (humanoid) {
                 actorLoop->memory->WriteMemory<float>(humanoid + Offsets::Humanoid::Walkspeed, settings.originalSpeed);
                 actorLoop->memory->WriteMemory<float>(humanoid + Offsets::Humanoid::WalkspeedCheck, settings.originalSpeed);
                 settings.lastWrittenSpeed = -1.0f;
            }
        }
    }
}

void Misc::UpdateGravity(ActorLoopClass* actorLoop) {
    if (!actorLoop->memory) return;
    
    // Feature disabled and code reverted to basic original structure as requested
    /*
    if (settings.enableGravity) {
        uintptr_t workspace = actorLoop->GetWorkspace();
        if (workspace) {
            actorLoop->memory->WriteMemory<float>(workspace + Offsets::Workspace::Gravity, settings.gravity);
        }
    }
    */
    
    // Jump Power Logic (kept as it was simple and mostly working)
    if (settings.enableJumpPower) {
        uintptr_t humanoid = actorLoop->GetLocalHumanoid();
        if (humanoid) {
            float targetJump = settings.jumpPower;
            actorLoop->memory->WriteMemory<float>(humanoid + Offsets::Humanoid::JumpPower, targetJump);
            actorLoop->memory->WriteMemory<float>(humanoid + Offsets::Humanoid::JumpHeight, targetJump / 10.0f);
        }
    }
}
