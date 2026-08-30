#pragma once
#include "Config.h"

// Player select screen: two-phase P1/P2 roster pick with keyboard and mouse.

// Load/unload portrait icons and UI fonts.
bool LoadPlayerSelectTextures();
void CleanUpPlayerSelectTextures();

// D3D device reset helpers (fonts only; textures are D3DPOOL_MANAGED).
void NotifyPlayerSelectDeviceLost();
void NotifyPlayerSelectDeviceReset();

// choice: 0 = browsing, 1 = both P1 and P2 confirmed, 2 = go back (Esc from P1 phase)
void playerSelectScreen(int& choice);

// Reset edge-trigger held state and return to P1 phase (call when entering this screen).
void ResetPlayerSelectInputState();
