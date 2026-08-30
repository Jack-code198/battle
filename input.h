#pragma once
#include "Config.h"

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

// Lecture 4 KEYDOWN macro — buffer[key] & 0x80.
#define KEYDOWN(keyBuffer, key) ((keyBuffer)[(key)] & 0x80)

// Shared focus-aware queries used by all fighters (routes through InputManager).
bool IsGameKeyDown(int directInputKey);
bool IsGameMouseDown(int virtualKey);

// Menus, pause, mini game: keyboard via InputManager (no battle-input gate).
bool IsUiKeyDown(int directInputKey);

// CPU AI key overlay — while enabled, IsGameKeyDown / IsGameMouseDown use ONLY AI
// state (human keyboard/mouse are ignored) so P2 cannot mirror P1 skill keys.
void BeginAiInput();
void EndAiInput();
void ClearAiInput();
void SetAiKeyDown(int directInputKey, bool down);
void SetAiMouseLeftDown(bool down);
bool IsAiInputEnabled();

// RAII: enables AI overlay for the duration of a CPU UpdateHuman call.
struct AiInputScope {
    AiInputScope() { BeginAiInput(); ClearAiInput(); }
    ~AiInputScope() { ClearAiInput(); EndAiInput(); }
    AiInputScope(const AiInputScope&) = delete;
    AiInputScope& operator=(const AiInputScope&) = delete;
};

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
