#pragma once
#include "Config.h"

bool LoadMiniGameAssets();
void CleanUpMiniGameAssets();
void ResetMiniGameState();

// choice: 0 = playing, 2 = return to main menu (Esc)
void miniGameScreen(int& choice);
