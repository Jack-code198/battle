#pragma once
#include "config.h"

LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void CreateDirectInput();
void CleanUpDirectInput();
void GetInput();
void CreateMyWindow();
bool WindowIsRunning();
void CleanUpWindow();
void ToggleFullscreen();

// Client mouse position mapped into logical SCREEN_WIDTH x SCREEN_HEIGHT space.
// Needed so menu hit-tests still work after borderless fullscreen stretch.
bool GetGameCursorPos(POINT& outPt);

// Loads assets/cursor/*.ani (or .cur). Returns false if none found.
bool LoadGameCursor();
void CleanUpGameCursor();