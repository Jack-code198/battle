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
LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) {
            g_WindowHasFocus = false;
            ZeroMemory(diKeys, sizeof(diKeys));
        }
        else {
            g_WindowHasFocus = true;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_KEYDOWN:
        switch (wParam) {
        case VK_ESCAPE:
            if (IsFullscreen()) {
                ToggleFullscreen();
            }
            break;
        case 'Q':
            PostQuitMessage(0);
            break;
        case 'F':
            ToggleFullscreen();
            break;
        case 'H':
            if (TRAINING_MODE) {
                g_Player1.health = g_Player1.GetMaxHealth();
                g_Player1.isDead = false;
                g_Player2.Reset();
                SyncBattleHudHealth(1, g_Player1.GetHealth(), g_Player1.GetMaxHealth());
                SyncBattleHudHealth(2, g_Player2.GetHealth(), g_Player2.GetMaxHealth());
            }
            else {
                g_Player2.Reset();
                SyncBattleHudHealth(2, g_Player2.GetHealth(), g_Player2.GetMaxHealth());
            }
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

void CreateMyWindow() {
    ZeroMemory(&msg, sizeof(msg));
    ZeroMemory(&wndClass, sizeof(wndClass));
    wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndClass.hInstance = GetModuleHandle(NULL);
    wndClass.lpfnWndProc = WindowProcedure;
    wndClass.lpszClassName = "StandardD3DWindowClass";
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClass(&wndClass);

    g_hWnd = CreateWindowEx(0, wndClass.lpszClassName, "Persona Framework", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1024, 768, NULL, NULL, GetModuleHandle(NULL), NULL);

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
    UnregisterClass(wndClass.lpszClassName, GetModuleHandle(NULL));
}