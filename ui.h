#pragma once
#include "config.h"

bool LoadHudTextures();
void CleanUpHudTextures();
void ResetBattleHud(int p1MaxHealth, int p2MaxHealth);
void SyncBattleHudHealth(int playerSlot, int health, int maxHealth);
void DrawBattleHud(
    LPD3DXSPRITE sprite,
    int p1Health,
    int p1MaxHealth,
    int p1Sp,
    int p1MaxSp,
    int p2Health,
    int p2MaxHealth,
    int p2Sp,
    int p2MaxSp);
