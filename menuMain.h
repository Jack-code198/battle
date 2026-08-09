#pragma once
#include "config.h"

bool LoadMenuTextures();
void CleanUpMenuTextures();
void drawMenuOptions();
void renderMainMenu();

// choice is taken by reference: 0 = still browsing, 1 = Start Game confirmed,
// 2 = Options confirmed. Exit is handled internally (posts WM_QUIT).
void mainMenu(int& choice);
// Reset the input state for the menu (e.g., when returning to the menu from another screen)
void ResetMenuInputState();
