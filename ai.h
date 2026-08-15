#pragma once
#include "player/Fighter.h"

// =============================================================================
// Simple CPU AI (BMCS2224) — drives P2 by injecting keys into InputManager overlay.
// Approach → attack pulse → occasional jump / side attack. No hardcoded RECTs.
// =============================================================================

// Write AI key/mouse state for this frame (call inside AiInputScope).
void DriveSimpleAi(Fighter& cpuFighter);

// True while CPU should stay on reaction path (intro / damage / recover).
bool IsCpuLockedInReaction(const Fighter& cpuFighter, int currentState);
