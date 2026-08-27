#pragma once
#include "config.h"

// Main menu screen: background, title font, and selectable options.

bool LoadMenuTextures();
void CleanUpMenuTextures();
void NotifyMenuDeviceLost();
void NotifyMenuDeviceReset();
void drawMenuOptions();
void renderMainMenu();

// Background image only (no title or menu options) — used by credits screen.
void renderMainMenuBackdrop();

// choice: 0 = browsing, 1 = Start Game, 2 = Options, 3 = Credits. Exit posts WM_QUIT.
void mainMenu(int& choice);
void ResetMenuInputState();
