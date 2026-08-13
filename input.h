#pragma once
#include "config.h"

// =============================================================================
// Input module (BMCS2224) — OO DirectInput wrapper + window helpers.
// =============================================================================

// DirectInput keyboard manager (reusable / maintainable).
class InputManager {
public:
    InputManager();
    ~InputManager();

    bool Create(HWND windowHandle);
    void Cleanup();
    void Poll(bool windowHasFocus);
    bool IsKeyDown(int directInputKey) const;
    const BYTE* GetKeyState() const { return keyState; }

private:
    LPDIRECTINPUT8 directInput;
    LPDIRECTINPUTDEVICE8 keyboardDevice;
    BYTE keyState[256];
};

extern InputManager g_InputManager;

// Shared focus-aware queries used by all fighters (routes through InputManager).
bool IsGameKeyDown(int directInputKey);
bool IsGameMouseDown(int virtualKey);

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
