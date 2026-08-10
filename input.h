#pragma once
#include "config.h"

// Window + DirectInput module (BMCS2224).
// Creates the Win32 window, polls the keyboard device into diKeys[], and maps mouse to game space.

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

// Custom cursor is only for Main Menu / Stage Select. Hide it in battle.
void SetMenuCursorEnabled(bool enabled);
bool IsMenuCursorEnabled();
