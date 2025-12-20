#include "misc.h"
#include "esp.h"
#include "../sdk/offsets.h"
#include <iostream>

Misc::Misc() {
}

void Misc::Update(ActorLoopClass* actorLoop) {
    if (!actorLoop) return;
    
    UpdateSpeed(actorLoop);
    UpdateGravity(actorLoop);
}

void Misc::UpdateSpeed(ActorLoopClass* actorLoop) {
    if (!actorLoop->memory) return;

    auto now = std::chrono::high_resolution_clock::now();
    const auto write_interval = std::chrono::milliseconds(500);

    if (settings.enableSpeed) {
        uintptr_t humanoid = actorLoop->GetLocalHumanoid();
        if (humanoid) {
            if (!settings.originalSpeedStored) {
                float currentSpeed = actorLoop->memory->ReadMemory<float>(humanoid + Offsets::Humanoid::Walkspeed);
                if (currentSpeed > 0.0f && currentSpeed < 1000.0f) {
                    settings.originalSpeed = currentSpeed;
                    settings.originalSpeedStored = true;
                }
            }

            float targetSpeed = settings.speed;
            if (targetSpeed < 1.0f) targetSpeed = 1.0f;
            if (targetSpeed > 100.0f) targetSpeed = 100.0f;

            bool valueChanged = (settings.lastWrittenSpeed < 0.0f) || (targetSpeed != settings.lastWrittenSpeed);
            bool enoughTimePassed = (settings.lastWrittenSpeed < 0.0f) || (now - settings.lastSpeedWrite > write_interval);

            if (valueChanged && enoughTimePassed) {
                actorLoop->memory->WriteMemory<float>(humanoid + Offsets::Humanoid::Walkspeed, targetSpeed);
                actorLoop->memory->WriteMemory<float>(humanoid + Offsets::Humanoid::WalkspeedCheck, targetSpeed);
                settings.lastWrittenSpeed = targetSpeed;
                settings.lastSpeedWrite = now;
            }
        }
    } else {
        if (settings.originalSpeedStored && settings.lastWrittenSpeed >= 0.0f && settings.lastWrittenSpeed != settings.originalSpeed) {
            uintptr_t humanoid = actorLoop->GetLocalHumanoid();
            if (humanoid && (now - settings.lastSpeedWrite > write_interval)) {
                 actorLoop->memory->WriteMemory<float>(humanoid + Offsets::Humanoid::Walkspeed, settings.originalSpeed);
                 actorLoop->memory->WriteMemory<float>(humanoid + Offsets::Humanoid::WalkspeedCheck, settings.originalSpeed);
                 settings.lastWrittenSpeed = -1.0f;
                 settings.lastSpeedWrite = now;
            }
        } else if (settings.lastWrittenSpeed >= 0.0f) {
            settings.lastWrittenSpeed = -1.0f;
        }
    }
}

void Misc::UpdateGravity(ActorLoopClass* actorLoop) {
    if (!actorLoop->memory) return;
    
    auto now = std::chrono::high_resolution_clock::now();
    const auto write_interval = std::chrono::milliseconds(500);

    if (settings.enableGravity) {
        uintptr_t workspace = actorLoop->GetWorkspace();
        if (workspace) {
            if (!settings.originalGravityStored) {
                float currentGravity = actorLoop->memory->ReadMemory<float>(workspace + Offsets::Workspace::Gravity);
                if (currentGravity > 0.0f && currentGravity < 1000.0f) {
                    settings.originalGravity = currentGravity;
                    settings.originalGravityStored = true;
                }
            }

            float targetGravity = settings.gravity;
            if (targetGravity < 0.0f) targetGravity = 0.0f;
            if (targetGravity > 500.0f) targetGravity = 500.0f;

            bool valueChanged = (settings.lastWrittenGravity < 0.0f) || (targetGravity != settings.lastWrittenGravity);
            bool enoughTimePassed = (settings.lastWrittenGravity < 0.0f) || (now - settings.lastGravityWrite > write_interval);

            if (valueChanged && enoughTimePassed) {
                actorLoop->memory->WriteMemory<float>(workspace + Offsets::Workspace::Gravity, targetGravity);
                settings.lastWrittenGravity = targetGravity;
                settings.lastGravityWrite = now;
            }
        }
    } else {
        if (settings.originalGravityStored && settings.lastWrittenGravity >= 0.0f && settings.lastWrittenGravity != settings.originalGravity) {
             uintptr_t workspace = actorLoop->GetWorkspace();
             if (workspace && (now - settings.lastGravityWrite > write_interval)) {
                 actorLoop->memory->WriteMemory<float>(workspace + Offsets::Workspace::Gravity, settings.originalGravity);
                 settings.lastWrittenGravity = -1.0f;
                 settings.lastGravityWrite = now;
             }
        } else if (settings.lastWrittenGravity >= 0.0f) {
            settings.lastWrittenGravity = -1.0f;
        }
    }
    
    if (settings.enableJumpPower) {
        uintptr_t humanoid = actorLoop->GetLocalHumanoid();
        if (humanoid) {
             if (!settings.originalJumpPowerStored) {
                float currentJump = actorLoop->memory->ReadMemory<float>(humanoid + Offsets::Humanoid::JumpPower);
                if (currentJump > 0.0f && currentJump < 1000.0f) {
                    settings.originalJumpPower = currentJump;
                    settings.originalJumpPowerStored = true;
                }
            }
            
            float targetJump = settings.jumpPower;
            
            bool valueChanged = (settings.lastWrittenJumpPower < 0.0f) || (targetJump != settings.lastWrittenJumpPower);
            bool enoughTimePassed = (settings.lastWrittenJumpPower < 0.0f) || (now - settings.lastJumpPowerWrite > write_interval);
            
             if (valueChanged && enoughTimePassed) {
                actorLoop->memory->WriteMemory<float>(humanoid + Offsets::Humanoid::JumpPower, targetJump);
                settings.lastWrittenJumpPower = targetJump;
                settings.lastJumpPowerWrite = now;
            }
        }
    } else {
         if (settings.originalJumpPowerStored && settings.lastWrittenJumpPower >= 0.0f && settings.lastWrittenJumpPower != settings.originalJumpPower) {
             uintptr_t humanoid = actorLoop->GetLocalHumanoid();
             if (humanoid && (now - settings.lastJumpPowerWrite > write_interval)) {
                 actorLoop->memory->WriteMemory<float>(humanoid + Offsets::Humanoid::JumpPower, settings.originalJumpPower);
                 settings.lastWrittenJumpPower = -1.0f;
                 settings.lastJumpPowerWrite = now;
             }
         } else if (settings.lastWrittenJumpPower >= 0.0f) {
            settings.lastWrittenJumpPower = -1.0f;
         }
    }
}
