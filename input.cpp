#include "input.h"
#include "renderer.h"
#include "game_logic.h"
#include "audio.h"
#include "ui.h"

HWND g_hWnd = NULL;
WNDCLASS wndClass;
MSG msg;
LPDIRECTINPUT8 dInput = NULL;
LPDIRECTINPUTDEVICE8 dInputKeyboardDevice = NULL;
BYTE diKeys[256];
bool g_WindowHasFocus = true;
static HCURSOR g_GameCursor = NULL;
static bool g_MenuCursorEnabled = true;

static void ApplyMenuCursorState() {
    if (!g_hWnd) return;

    if (g_MenuCursorEnabled) {
        if (g_GameCursor) {
            SetClassLongPtr(g_hWnd, GCLP_HCURSOR, (LONG_PTR)g_GameCursor);
            SetCursor(g_GameCursor);
        }
        while (ShowCursor(TRUE) < 0) {
        }
    }
    else {
        SetClassLongPtr(g_hWnd, GCLP_HCURSOR, NULL);
        SetCursor(NULL);
        while (ShowCursor(FALSE) >= 0) {
        }
    }
}

void SetMenuCursorEnabled(bool enabled) {
    if (g_MenuCursorEnabled == enabled) {
        return;
    }
    g_MenuCursorEnabled = enabled;
    ApplyMenuCursorState();
}

bool IsMenuCursorEnabled() {
    return g_MenuCursorEnabled;
}

LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) {
            g_WindowHasFocus = false;
            ZeroMemory(diKeys, sizeof(diKeys));
        }
        else {
            g_WindowHasFocus = true;
            // Re-apply cursor policy when regaining focus.
            ApplyMenuCursorState();
        }
        break;
    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT) {
            if (g_MenuCursorEnabled && g_GameCursor) {
                SetCursor(g_GameCursor);
                return TRUE;
            }
            if (!g_MenuCursorEnabled) {
                SetCursor(NULL);
                return TRUE;
            }
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_KEYDOWN:
        switch (wParam) {
        case VK_ESCAPE:
            // Fullscreen toggle is on F. Escape is used by menus / battle-to-menu.
            break;
        case 'Q':
            PostQuitMessage(0);
            break;
        case 'F':
            ToggleFullscreen();
            break;
        case 'H':
            // Training heal only — do NOT Reset() (that teleports P2 and replays intro).
            if (!g_Player1 || !g_Player2) break;
            g_Player1->health = g_Player1->GetMaxHealth();
            g_Player1->isDead = false;
            g_Player2->health = g_Player2->GetMaxHealth();
            g_Player2->isDead = false;
            SyncBattleHudHealth(1, g_Player1->GetHealth(), g_Player1->GetMaxHealth());
            SyncBattleHudHealth(2, g_Player2->GetHealth(), g_Player2->GetMaxHealth());
            break;
        case 'B':
            g_ShowDebugHitboxes = !g_ShowDebugHitboxes;
            break;
        case 'M':
            g_SoundManager.ToggleMusicMute();
            break;
        }
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void CreateDirectInput() {
    DirectInput8Create(GetModuleHandle(NULL), 0x0800, IID_IDirectInput8, (void**)&dInput, NULL);
    dInput->CreateDevice(GUID_SysKeyboard, &dInputKeyboardDevice, NULL);
    dInputKeyboardDevice->SetDataFormat(&c_dfDIKeyboard);
    dInputKeyboardDevice->SetCooperativeLevel(g_hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
    dInputKeyboardDevice->Acquire();
}

void CleanUpDirectInput() {
    if (dInputKeyboardDevice) {
        dInputKeyboardDevice->Unacquire();
        dInputKeyboardDevice->Release();
        dInputKeyboardDevice = NULL;
    }
    if (dInput) {
        dInput->Release();
        dInput = NULL;
    }
}

void GetInput() {
    if (!g_WindowHasFocus) {
        ZeroMemory(diKeys, sizeof(diKeys));
        return;
    }

    if (dInputKeyboardDevice) {
        HRESULT hr = dInputKeyboardDevice->GetDeviceState(256, diKeys);
        if (FAILED(hr)) {
            dInputKeyboardDevice->Acquire();
        }
    }
}

bool LoadGameCursor() {
    if (g_GameCursor) {
        return true;
    }

    const char* cursorCandidates[] = {
        "assets/cursor/cursor.ani",
        "assets/cursor/cursor.cur",
        "assets/cursor/LINK SELECT.ani",
        "assets/cursor/link_select.ani"
    };

    for (const char* path : cursorCandidates) {
        HCURSOR loaded = LoadCursorFromFileA(path);
        if (loaded) {
            g_GameCursor = loaded;
            if (g_hWnd) {
                SetClassLongPtr(g_hWnd, GCLP_HCURSOR, (LONG_PTR)g_GameCursor);
                SetCursor(g_GameCursor);
            }
            return true;
        }
    }

    return false;
}

void CleanUpGameCursor() {
    if (g_GameCursor) {
        DestroyCursor(g_GameCursor);
        g_GameCursor = NULL;
    }
}

void CreateMyWindow() {
    ZeroMemory(&msg, sizeof(msg));
    ZeroMemory(&wndClass, sizeof(wndClass));
    wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndClass.hInstance = GetModuleHandle(NULL);
    wndClass.lpfnWndProc = WindowProcedure;
    wndClass.lpszClassName = "StandardD3DWindowClass";
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wndClass);

    // Size the outer window so the client area is exactly SCREEN_WIDTH x SCREEN_HEIGHT.
    RECT windowRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
    const int windowW = windowRect.right - windowRect.left;
    const int windowH = windowRect.bottom - windowRect.top;

    g_hWnd = CreateWindowEx(0, wndClass.lpszClassName, "Persona Framework", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, windowW, windowH, NULL, NULL, GetModuleHandle(NULL), NULL);

    LoadGameCursor();
    SetMenuCursorEnabled(true);

    ShowWindow(g_hWnd, SW_SHOWNORMAL);
    UpdateWindow(g_hWnd);
}

bool WindowIsRunning() {
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) return false;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return true;
}

void CleanUpWindow() {
    CleanUpGameCursor();
    UnregisterClass(wndClass.lpszClassName, GetModuleHandle(NULL));
}

bool GetGameCursorPos(POINT& outPt) {
    if (!GetCursorPos(&outPt)) return false;
    if (!ScreenToClient(g_hWnd, &outPt)) return false;

    RECT clientRect = {};
    if (!GetClientRect(g_hWnd, &clientRect)) return false;

    const int clientW = clientRect.right - clientRect.left;
    const int clientH = clientRect.bottom - clientRect.top;
    if (clientW <= 0 || clientH <= 0) return false;

    // Present stretches the backbuffer to the client; map mouse into game coords.
    outPt.x = (LONG)((outPt.x * (LONGLONG)SCREEN_WIDTH) / clientW);
    outPt.y = (LONG)((outPt.y * (LONGLONG)SCREEN_HEIGHT) / clientH);
    return true;
}