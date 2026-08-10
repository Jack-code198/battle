#pragma once
#include "config.h"

// Main menu screen: background, title font, and selectable options.

bool LoadMenuTextures();
void CleanUpMenuTextures();
void drawMenuOptions();
void renderMainMenu();

// choice: 0 = browsing, 1 = Start Game, 2 = Options. Exit posts WM_QUIT.
void mainMenu(int& choice);
void ResetMenuInputState();
