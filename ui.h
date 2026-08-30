#pragma once
#include "Config.h"

bool LoadHudTextures();
void CleanUpHudTextures();
void NotifyHudDeviceLost();
void NotifyHudDeviceReset();
void ResetBattleHud(int p1MaxHealth, int p2MaxHealth);
void SyncBattleHudHealth(int playerSlot, int health, int maxHealth);
void ForceSyncBattleHudHealth(int playerSlot, int health, int maxHealth);
void DrawBattleHud(
    LPD3DXSPRITE sprite,
    int p1Health,
    int p1MaxHealth,
    int p1Sp,
    int p1MaxSp,
    float p1Stamina,
    float p1MaxStamina,
    int p2Health,
    int p2MaxHealth,
    int p2Sp,
    int p2MaxSp,
    float p2Stamina,
    float p2MaxStamina);
void DrawBattleRoundOverlay();
void DrawBattleTimerOverlay();
void DrawHitComboOverlay();
void DrawBattleFadeOverlay(LPD3DXSPRITE sprite);
void DrawTutorialGuideOverlay();
void DrawBattleDebugHintOverlay();
