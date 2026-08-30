#pragma once
#include "Config.h"

// Options screen: simple vertical list (Music toggle, Back). Reached from
// the main menu's "Options" entry.

bool LoadOptionsTextures();
void CleanUpOptionsTextures();
void NotifyOptionsDeviceLost();
void NotifyOptionsDeviceReset();

// choice: 0 = still browsing, 2 = go back (Esc/Backspace or "Back" selected)
void optionsMenuScreen(int& choice);
void ResetOptionsMenuInputState();
