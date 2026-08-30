#pragma once
#include "Config.h"

// Layered battle backgrounds with parallax tied to the human-controlled fighter.

bool LoadBattleParallaxForStage(int stageIndex);
void CleanUpBattleParallax();
void ResetBattleParallaxScroll();
void UpdateBattleParallaxScroll();
void DrawBattleParallaxBackground(LPD3DXSPRITE sprite);
bool BattleParallaxBackgroundReady();
