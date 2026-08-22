#include "ai.h"
#include "config.h"
#include "game_logic.h"
#include "input.h"
#include "player/makoto/Makoto.h"
#include "player/joker/Joker.h"
#include "player/narukami/Narukami.h"
#include "player/yosuke/Yosuke.h"
#include <cstdlib>
#include <cmath>

struct AiBrain {
    static void SetToward(float deltaX) {
        if (deltaX > 0.0f) {
            SetAiKeyDown(DIK_D, true);
            SetAiKeyDown(DIK_RIGHT, true);
        }
        else {
            SetAiKeyDown(DIK_A, true);
            SetAiKeyDown(DIK_LEFT, true);
        }
    }

    static void SetAway(const Fighter& self) {
        const int away = GetGuardAwayDirection(self);
        if (away < 0) {
            SetAiKeyDown(DIK_LEFT, true);
            SetAiKeyDown(DIK_A, true);
        }
        else {
            SetAiKeyDown(DIK_RIGHT, true);
            SetAiKeyDown(DIK_D, true);
        }
    }

    static void PickAttackMode(Fighter& cpu) {
        const int roll = rand() % 100;
        // Narukami CPU: prefer basic LMB — E/R need stamina and can silently fail.
        if (cpu.GetCharacterId() == Char_Narukami) {
            if (roll < 10) {
                cpu.aiAttackMode = 1;
            }
            else if (roll < 18) {
                cpu.aiAttackMode = 2;
            }
            else {
                cpu.aiAttackMode = 0;
            }
            return;
        }
        if (roll < AI_SIDE_ATTACK_CHANCE_PERCENT) {
            cpu.aiAttackMode = 1; // E — held key, reliable for CPU
        }
        else if (roll < AI_SIDE_ATTACK_CHANCE_PERCENT + 15) {
            cpu.aiAttackMode = 2; // R
        }
        else {
            cpu.aiAttackMode = 0; // LMB tap
        }
    }

    static void FireAttackInput(Fighter& cpu) {
        if (cpu.aiAttackMode == 1) {
            SetAiKeyDown(DIK_E, true);
        }
        else if (cpu.aiAttackMode == 2) {
            SetAiKeyDown(DIK_R, true);
        }
        else {
            // Fighters use edge-detect for LMB — single-frame tap only.
            SetAiMouseLeftDown(true);
        }
    }

    static void BeginAttack(Fighter& cpu) {
        PickAttackMode(cpu);
        cpu.aiAttackCooldown = AI_ATTACK_COOLDOWN_STEPS;
        cpu.aiAttackPulse = (cpu.aiAttackMode == 0)
            ? AI_ATTACK_PULSE_STEPS
            : AI_SKILL_ATTACK_PULSE_STEPS;
        cpu.aiMovePulse = 0;
        cpu.aiMoveIntent = 0;
    }

    static void TryAttack(Fighter& cpu) {
        if (cpu.aiAttackCooldown > 0) return;
        BeginAttack(cpu);
        FireAttackInput(cpu);
    }

    static void MaintainSpacing(Fighter& cpu, float distanceX) {
        if (distanceX < AI_SPACING_RANGE) {
            SetAway(cpu);
        }
    }

    static void Tick(Fighter& cpu) {
        Fighter* enemy = GetOpponent(cpu);
        if (!enemy || enemy->IsDead() || cpu.IsDead()) return;
        if (!IsBattleInputAllowed()) {
            ClearAiInput();
            return;
        }

        ClearAiInput();

        const float deltaX = enemy->GetPosition().x - cpu.GetPosition().x;
        const float distanceX = fabsf(deltaX);

        if (cpu.aiAttackCooldown > 0) --cpu.aiAttackCooldown;
        if (cpu.aiJumpCooldown > 0) --cpu.aiJumpCooldown;

        if (cpu.aiAttackPulse > 0) {
            --cpu.aiAttackPulse;
            FireAttackInput(cpu);
            return;
        }

        const bool inMeleeRange = distanceX <= AI_ATTACK_RANGE;
        const bool canApproach = distanceX > (AI_ATTACK_RANGE + AI_APPROACH_STOP_GAP);

        if (inMeleeRange) {
            TryAttack(cpu);
            if (cpu.aiAttackPulse <= 0) {
                MaintainSpacing(cpu, distanceX);
            }
            return;
        }

        if (canApproach) {
            if (cpu.aiMovePulse <= 0) {
                cpu.aiMoveIntent = (distanceX > AI_RUN_RANGE) ? 2 : 1;
                cpu.aiMovePulse = AI_APPROACH_BURST_STEPS + (rand() % 4);
            }
            else {
                --cpu.aiMovePulse;
            }

            SetToward(deltaX);
            if (cpu.aiMoveIntent == 2) {
                SetAiKeyDown(DIK_LSHIFT, true);
            }

            if (cpu.aiJumpCooldown <= 0 &&
                distanceX > AI_ATTACK_RANGE &&
                distanceX < AI_ENGAGE_RANGE &&
                (rand() % 100) < AI_JUMP_CHANCE_PERCENT) {
                SetAiKeyDown(DIK_SPACE, true);
                cpu.aiJumpCooldown = AI_JUMP_COOLDOWN_STEPS;
            }
            return;
        }

        // Spacing band: close enough to fight, stop running in.
        cpu.aiMovePulse = 0;
        TryAttack(cpu);
        if (cpu.aiAttackPulse <= 0) {
            MaintainSpacing(cpu, distanceX);
        }
    }
};

bool IsCpuLockedInReaction(const Fighter& cpuFighter, int currentState) {
    if (cpuFighter.IsPlayingResultPose()) return true;

    switch (cpuFighter.GetCharacterId()) {
    case Char_Makoto:
        return currentState == INTRO ||
            currentState == DAMAGE ||
            currentState == RECOVER ||
            currentState == MAKOTO_WIN ||
            currentState == THANATOS_WIN ||
            currentState == MAKOTO_LOSE;
    case Char_Joker:
        return currentState == JOKER_INTRO ||
            currentState == JOKER_DAMAGE ||
            currentState == JOKER_RECOVER ||
            currentState == JOKER_WIN ||
            currentState == JOKER_LOSE;
    case Char_Narukami:
        return currentState == NARUKAMI_INTRO ||
            currentState == NARUKAMI_INTRO_DISCARD ||
            currentState == NARUKAMI_DAMAGE ||
            currentState == NARUKAMI_RECOVER ||
            currentState == NARUKAMI_WIN ||
            currentState == NARUKAMI_LOSE;
    case Char_Yosuke:
        return currentState == YOSUKE_INTRO ||
            currentState == YOSUKE_DAMAGE ||
            currentState == YOSUKE_RECOVER ||
            currentState == YOSUKE_WIN ||
            currentState == YOSUKE_LOSE;
    default:
        return false;
    }
}

void DriveSimpleAi(Fighter& cpuFighter) {
    AiBrain::Tick(cpuFighter);
}
