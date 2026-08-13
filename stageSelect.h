#pragma once
#include "config.h"

// Stage select screen: preview textures, keyboard/mouse navigation, confirm/back.

struct StageInfo {
    const char* name;
    const char* texturePath;
    LPDIRECT3DTEXTURE9 texture; // loaded by LoadStageTextures()
};

// Edit this list (in stageSelect.cpp) to add/rename/repoint stages.
extern StageInfo g_Stages[];
extern const int STAGE_COUNT;
extern int g_SelectedStageIndex;

bool LoadStageTextures();
void CleanUpStageTextures();
void NotifyStageDeviceLost();
void NotifyStageDeviceReset();

// choice: 0 = still browsing, 1 = stage confirmed (Enter), 2 = go back (Esc/Backspace)
void stageSelectScreen(int& choice);

// Points texBgCity1 (the battle background Render() already draws) at
// whichever stage is currently selected. Call after choice == 1.
void ApplySelectedStageToBattle();
// Reset the input state for the stage select screen (e.g., when returning to it from another screen)
void ResetStageSelectInputState();
