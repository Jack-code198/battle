#include "Renderer.h"
#include "FontRenderer.h"
#include "GameLogic.h"
#include "UI.h"
#include "OptionMenu.h"
#include "PauseMenu.h"
#include "MainMenu.h"
#include "StageSelect.h"
#include "BattleBackground.h"
#include "PlayerSelect.h"
#include "player/makoto/Makoto.h"
#include "player/joker/Joker.h"
#include "player/narukami/Narukami.h"
#include "player/yosuke/Yosuke.h"
#include <d3dx9.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

extern LPDIRECT3DTEXTURE9 texBgCity1;
IDirect3D9* g_pD3D = NULL;
IDirect3DDevice9* g_pD3DDevice = NULL;
D3DPRESENT_PARAMETERS d3dpp;
LPD3DXSPRITE spriteBrush = NULL;

static bool g_IsBorderlessFullscreen = false;
static bool g_PendingFullscreenToggle = false;
static bool g_PendingDeviceReset = false;
static RECT g_WindowedRect = {};
static LONG g_WindowedStyle = 0;
static LONG g_WindowedExStyle = 0;

static void PumpWindowMessages() {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

static void NotifyAllDeviceLost() {
    if (spriteBrush) spriteBrush->OnLostDevice();
    NotifyGameFontDeviceLost();
    NotifyMenuDeviceLost();
    NotifyStageDeviceLost();
    NotifyPlayerSelectDeviceLost();
    NotifyHudDeviceLost();
    NotifyOptionsDeviceLost();
    NotifyPauseMenuDeviceLost();
}

static void NotifyAllDeviceReset() {
    if (spriteBrush) {
        if (FAILED(spriteBrush->OnResetDevice())) {
            spriteBrush->Release();
            spriteBrush = NULL;
            D3DXCreateSprite(g_pD3DDevice, &spriteBrush);
        }
    }
    else if (g_pD3DDevice) {
        D3DXCreateSprite(g_pD3DDevice, &spriteBrush);
    }

    NotifyGameFontDeviceReset(g_pD3DDevice);
    NotifyMenuDeviceReset();
    NotifyStageDeviceReset();
    NotifyPlayerSelectDeviceReset();
    NotifyHudDeviceReset();
    NotifyOptionsDeviceReset();
    NotifyPauseMenuDeviceReset();
}

static bool ResetGraphicsDevice() {
    if (!g_pD3DDevice || !g_hWnd) return false;

    NotifyAllDeviceLost();

    d3dpp.Windowed = TRUE;
    d3dpp.BackBufferWidth = SCREEN_WIDTH;
    d3dpp.BackBufferHeight = SCREEN_HEIGHT;
    d3dpp.hDeviceWindow = g_hWnd;
    d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    const HRESULT hr = g_pD3DDevice->Reset(&d3dpp);
    if (FAILED(hr)) {
        return false;
    }

    NotifyAllDeviceReset();
    WarmupRenderPipeline();
    return true;
}

static void ApplyFullscreenWindowChange() {
    if (g_hWnd == NULL) return;

    if (!g_IsBorderlessFullscreen) {
        GetWindowRect(g_hWnd, &g_WindowedRect);
        g_WindowedStyle = GetWindowLong(g_hWnd, GWL_STYLE);
        g_WindowedExStyle = GetWindowLong(g_hWnd, GWL_EXSTYLE);

        HMONITOR monitor = MonitorFromWindow(g_hWnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monitorInfo = {};
        monitorInfo.cbSize = sizeof(MONITORINFO);
        GetMonitorInfo(monitor, &monitorInfo);

        SetWindowLong(g_hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowLong(g_hWnd, GWL_EXSTYLE, g_WindowedExStyle);
        SetWindowPos(
            g_hWnd,
            HWND_TOP,
            monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.top,
            monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
            SWP_FRAMECHANGED | SWP_SHOWWINDOW);

        g_IsBorderlessFullscreen = true;
    }
    else {
        SetWindowLong(g_hWnd, GWL_STYLE, g_WindowedStyle);
        SetWindowLong(g_hWnd, GWL_EXSTYLE, g_WindowedExStyle);
        SetWindowPos(
            g_hWnd,
            HWND_NOTOPMOST,
            g_WindowedRect.left,
            g_WindowedRect.top,
            g_WindowedRect.right - g_WindowedRect.left,
            g_WindowedRect.bottom - g_WindowedRect.top,
            SWP_FRAMECHANGED | SWP_SHOWWINDOW);

        g_IsBorderlessFullscreen = false;
    }
}

void DrawDebugRect(LPD3DXSPRITE sprite, float x, float y, float w, float h, D3DCOLOR color) {
    if (!sprite) return;

    D3DXMATRIX identity;
    D3DXMatrixIdentity(&identity);
    sprite->SetTransform(&identity);
    sprite->Flush();

    struct Vertex { FLOAT x, y, z, rhw; DWORD color; };
    Vertex vertices[4] = {
        { x, y, 0.0f, 1.0f, color },
        { x + w, y, 0.0f, 1.0f, color },
        { x, y + h, 0.0f, 1.0f, color },
        { x + w, y + h, 0.0f, 1.0f, color }
    };

    LPDIRECT3DDEVICE9 device = nullptr;
    sprite->GetDevice(&device);
    if (!device) return;

    device->SetTexture(0, NULL);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex));
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device->Release();
}

void DrawDebugCircleRing(
    LPD3DXSPRITE sprite,
    float cx,
    float cy,
    float radius,
    D3DCOLOR color,
    int segments)
{
    if (!sprite || radius <= 0.0f || segments < 8) return;

    D3DXMATRIX identity;
    D3DXMatrixIdentity(&identity);
    sprite->SetTransform(&identity);
    sprite->Flush();

    struct Vertex { FLOAT x, y, z, rhw; DWORD color; };
    const int vertCount = segments + 1;
    Vertex vertices[65];
    if (vertCount > 65) return;

    for (int i = 0; i < vertCount; ++i) {
        const float angle = ((float)i / (float)segments) * (float)(2.0 * M_PI);
        vertices[i].x = cx + cosf(angle) * radius;
        vertices[i].y = cy + sinf(angle) * radius;
        vertices[i].z = 0.0f;
        vertices[i].rhw = 1.0f;
        vertices[i].color = color;
    }

    LPDIRECT3DDEVICE9 device = nullptr;
    sprite->GetDevice(&device);
    if (!device) return;

    device->SetTexture(0, NULL);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
    device->DrawPrimitiveUP(D3DPT_LINESTRIP, segments, vertices, sizeof(Vertex));
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device->Release();
}

bool InitD3D() {
    g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (g_pD3D == NULL) return false;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
    d3dpp.BackBufferWidth = SCREEN_WIDTH;
    d3dpp.BackBufferHeight = SCREEN_HEIGHT;
    d3dpp.hDeviceWindow = g_hWnd;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    DWORD behaviorFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;
    HRESULT hr = g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, g_hWnd,
        behaviorFlags, &d3dpp, &g_pD3DDevice);
    if (FAILED(hr)) {
        behaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        hr = g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, g_hWnd,
            behaviorFlags, &d3dpp, &g_pD3DDevice);
    }
    return SUCCEEDED(hr);
}

void RenderBattleSceneContents() {
    if (spriteBrush == NULL) return;

    spriteBrush->Begin(D3DXSPRITE_ALPHABLEND);

    const bool ultimateActive =
        (g_Player1 && g_Player1->IsSuperMoveActive()) ||
        (g_Player2 && g_Player2->IsSuperMoveActive());

    if (!ultimateActive) {
        DrawBattleParallaxBackground(spriteBrush);
    }
    else {
        DrawBattleParallaxBackground(spriteBrush);
        DrawDebugRect(
            spriteBrush,
            0.0f,
            0.0f,
            (float)SCREEN_WIDTH,
            (float)SCREEN_HEIGHT,
            D3DCOLOR_ARGB(255, 0, 0, 0));
    }

    if (g_Player1 && g_Player2) {
        ApplyBattleRenderTints();
        EnsureBattleResultPosesApplied();
        Fighter* leftFighter = g_Player1;
        Fighter* rightFighter = g_Player2;

        rightFighter->RenderSkillBackdropBeforeOpponent(spriteBrush);
        leftFighter->Render(spriteBrush);
        leftFighter->RenderSkillBackdropBeforeOpponent(spriteBrush);
        rightFighter->Render(spriteBrush);

        if (g_ShowDebugHitboxes && !g_SuppressBattleDebugOverlay) {
            g_Player1->RenderDebugHitbox(spriteBrush);
            g_Player2->RenderDebugHitbox(spriteBrush);
        }
    }

    spriteBrush->End();

    DrawBattleRoundOverlay();
    DrawBattleTimerOverlay();
    DrawHitComboOverlay();
    DrawBattleFadeOverlay(spriteBrush);

    if (g_Player1 && g_Player2) {
        DrawBattleHud(
            spriteBrush,
            g_Player1->GetHealth(),
            g_Player1->GetMaxHealth(),
            g_Player1->GetSp(),
            g_Player1->GetMaxSp(),
            g_Player1->GetStamina(),
            g_Player1->GetMaxStamina(),
            g_Player2->GetHealth(),
            g_Player2->GetMaxHealth(),
            g_Player2->GetSp(),
            g_Player2->GetMaxSp(),
            g_Player2->GetStamina(),
            g_Player2->GetMaxStamina());
    }

    DrawTutorialGuideOverlay();
    DrawBattleDebugHintOverlay();
}

void ApplyBrightnessOverlay() {
    if (!spriteBrush) return;
    if (g_BrightnessLevel == BRIGHTNESS_DEFAULT) return;

    int diff = g_BrightnessLevel - BRIGHTNESS_DEFAULT;
    D3DCOLOR overlayColor;
    if (diff < 0) {
        int range = BRIGHTNESS_DEFAULT - BRIGHTNESS_MIN;
        int alpha = (range > 0) ? (-diff * 255 / range) : 0;
        if (alpha > 255) alpha = 255;
        overlayColor = D3DCOLOR_ARGB(alpha, 0, 0, 0);
    }
    else {
        int range = BRIGHTNESS_MAX - BRIGHTNESS_DEFAULT;
        int alpha = (range > 0) ? (diff * 255 / range) : 0;
        if (alpha > 255) alpha = 255;
        overlayColor = D3DCOLOR_ARGB(alpha, 255, 255, 255);
    }

    spriteBrush->Begin(D3DXSPRITE_ALPHABLEND);
    DrawDebugRect(spriteBrush, 0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, overlayColor);
    spriteBrush->End();
}

void Render() {
    if (g_pD3DDevice == NULL) return;

    const HRESULT coop = g_pD3DDevice->TestCooperativeLevel();
    if (coop == D3DERR_DEVICELOST) {
        g_PendingDeviceReset = true;
        return;
    }
    if (coop == D3DERR_DEVICENOTRESET) {
        if (!ResetGraphicsDevice()) {
            g_PendingDeviceReset = true;
            return;
        }
        g_PendingDeviceReset = false;
    }

    g_pD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

    if (SUCCEEDED(g_pD3DDevice->BeginScene())) {
        RenderBattleSceneContents();
        ApplyBrightnessOverlay();
        g_pD3DDevice->EndScene();
    }
    const HRESULT presentHr = g_pD3DDevice->Present(NULL, NULL, NULL, NULL);
    if (presentHr == D3DERR_DEVICELOST) {
        g_PendingDeviceReset = true;
    }
}

void WarmupRenderPipeline() {
    if (!g_pD3DDevice) return;

    g_pD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    if (SUCCEEDED(g_pD3DDevice->BeginScene())) {
        if (spriteBrush && BattleParallaxBackgroundReady()) {
            spriteBrush->Begin(D3DXSPRITE_ALPHABLEND);
            DrawBattleParallaxBackground(spriteBrush);
            spriteBrush->End();
        }
        else if (texBgCity1 && spriteBrush) {
            D3DSURFACE_DESC desc;
            texBgCity1->GetLevelDesc(0, &desc);
            if (desc.Width > 0 && desc.Height > 0) {
                const float scaleX = (float)SCREEN_WIDTH / (float)desc.Width;
                const float scaleY = (float)SCREEN_HEIGHT / (float)desc.Height;
                D3DXMATRIX matScale;
                D3DXMatrixScaling(&matScale, scaleX, scaleY, 1.0f);
                spriteBrush->Begin(D3DXSPRITE_ALPHABLEND);
                spriteBrush->SetTransform(&matScale);
                D3DXVECTOR3 bgPos(0.0f, 0.0f, 0.0f);
                spriteBrush->Draw(texBgCity1, NULL, NULL, &bgPos, D3DCOLOR_XRGB(255, 255, 255));
                D3DXMATRIX matIdentity;
                D3DXMatrixIdentity(&matIdentity);
                spriteBrush->SetTransform(&matIdentity);
                spriteBrush->End();
            }
        }
        g_pD3DDevice->EndScene();
    }
    g_pD3DDevice->Present(NULL, NULL, NULL, NULL);
}

void CleanUpD3D() {
    CleanUpMakotoTextures();
    CleanUpJokerTextures();
    CleanUpNarukamiTextures();
    CleanUpYosukeTextures();
    CleanUpHudTextures();

    CleanUpBattleParallax();

    if (texBgCity1) { texBgCity1->Release(); texBgCity1 = NULL; }

    if (spriteBrush != NULL) { spriteBrush->Release(); spriteBrush = NULL; }
    if (g_pD3DDevice != NULL) { g_pD3DDevice->Release(); g_pD3DDevice = NULL; }
    if (g_pD3D != NULL) { g_pD3D->Release(); g_pD3D = NULL; }
}

void ToggleFullscreen() {
    g_PendingFullscreenToggle = true;
}

void ProcessGraphicsDeviceEvents() {
    if (g_PendingFullscreenToggle) {
        g_PendingFullscreenToggle = false;
        ApplyFullscreenWindowChange();
        PumpWindowMessages();
        if (!ResetGraphicsDevice()) {
            g_PendingDeviceReset = true;
        }
        else {
            g_PendingDeviceReset = false;
        }
        return;
    }

    if (g_PendingDeviceReset) {
        if (ResetGraphicsDevice()) {
            g_PendingDeviceReset = false;
        }
        return;
    }

    if (!g_pD3DDevice) return;
    const HRESULT coop = g_pD3DDevice->TestCooperativeLevel();
    if (coop == D3DERR_DEVICENOTRESET) {
        if (ResetGraphicsDevice()) {
            g_PendingDeviceReset = false;
        }
        else {
            g_PendingDeviceReset = true;
        }
    }
}

bool IsFullscreen() {
    return g_IsBorderlessFullscreen;
}
