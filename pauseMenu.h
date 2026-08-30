#pragma once
#include "Config.h"
#include "player/CharacterId.h"

// Battle pause overlay (Resume / Move List / Options / Exit) and the blank
// Move List sub-screen. Both are drawn as a small centered panel over a
// dimmed background — the caller is expected to have already Cleared and
// BeginScene'd, and to have drawn the frozen battle scene underneath via
// RenderBattleSceneContents() (see renderer.h) before calling these.

bool LoadPauseMenuTextures();
void CleanUpPauseMenuTextures();
void NotifyPauseMenuDeviceLost();
void NotifyPauseMenuDeviceReset();

// choice: 0 = browsing; battle: 1=Resume, 2=Move List, 3=Options, 4=Player Select, 5=Exit
// tutorial adds AI toggle (handled in-menu, choice 4 unused) and shifts 4=Player Select, 5=Exit, 6=Exit mapping
void pauseMenuScreen(int& choice);
void ResetPauseMenuInputState();

// choice: 0 = browsing, 2 = back to pause menu (Esc/Backspace or "Back")
void moveListScreen(int& choice, CharacterId activeCharacter);
void ResetMoveListInputState();
