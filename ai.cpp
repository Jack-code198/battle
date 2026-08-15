#include "ai.h"
#include "config.h"
#include "game_logic.h"
#include "input.h"
#include "player/makoto/Makoto.h"
#include "player/joker/Joker.h"
#include "player/narukami/Narukami.h"
#include <cstdlib>
#include <cmath>

bool IsCpuLockedInReaction(const Fighter& cpuFighter, int currentState) {
    if (cpuFighter.IsHit()) return true;
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
    default:
        return false;
    }
}

void DriveSimpleAi(Fighter& cpuFighter) {
    Fighter* enemy = GetOpponent(cpuFighter);
    if (!enemy || enemy->IsDead() || cpuFighter.IsDead()) return;
    if (!IsBattleInputAllowed()) {
        ClearAiInput();
        return;
    }

    ClearAiInput();

    // Passiveive AI: if the human is idle, CPU stays idle too.
    if (!IsHumanPlayerEngaged()) {
        return;
    }

    if (cpuFighter.aiAttackCooldown > 0) --cpuFighter.aiAttackCooldown;
    if (cpuFighter.aiJumpCooldown > 0) --cpuFighter.aiJumpCooldown;
    if (cpuFighter.aiAttackPulse > 0) --cpuFighter.aiAttackPulse;

    const float deltaX = enemy->GetPosition().x - cpuFighter.GetPosition().x;
    const float distanceX = fabsf(deltaX);

    // Close the gap: walk / run toward the opponent.
    if (distanceX > AI_ATTACK_RANGE) {
        if (deltaX > 0.0f) {
            SetAiKeyDown(DIK_D, true);
            SetAiKeyDown(DIK_RIGHT, true);
        }
        else {
            SetAiKeyDown(DIK_A, true);
            SetAiKeyDown(DIK_LEFT, true);
        }
        if (distanceX > AI_RUN_RANGE) {
            SetAiKeyDown(DIK_LSHIFT, true);
        }
    }

    // Start a new attack when in range and off cooldown.
    if (distanceX <= AI_ATTACK_RANGE && cpuFighter.aiAttackCooldown <= 0) {
        cpuFighter.aiAttackCooldown = AI_ATTACK_COOLDOWN_STEPS;
        cpuFighter.aiAttackPulse = AI_ATTACK_PULSE_STEPS;
        const int roll = rand() % 100;
        if (roll < AI_SIDE_ATTACK_CHANCE_PERCENT) {
            cpuFighter.aiAttackMode = 1; // E
        }
        else if (roll < AI_SIDE_ATTACK_CHANCE_PERCENT * 2) {
            cpuFighter.aiAttackMode = 2; // R
        }
        else {
            cpuFighter.aiAttackMode = 0; // LMB
        }
    }

    // Hold the chosen attack for a few steps (edge-detect sees a press).
    if (cpuFighter.aiAttackPulse > 0) {
        if (cpuFighter.aiAttackMode == 1) {
            SetAiKeyDown(DIK_E, true);
        }
        else if (cpuFighter.aiAttackMode == 2) {
            SetAiKeyDown(DIK_R, true);
        }
        else {
            SetAiMouseLeftDown(true);
        }
    }

    // Occasional jump when mid-range.
    if (cpuFighter.aiJumpCooldown <= 0 &&
        distanceX < AI_ENGAGE_RANGE &&
        distanceX > AI_ATTACK_RANGE * 0.5f &&
        (rand() % 100) < AI_JUMP_CHANCE_PERCENT) {
        SetAiKeyDown(DIK_SPACE, true);
        cpuFighter.aiJumpCooldown = AI_JUMP_COOLDOWN_STEPS;
    }
}
