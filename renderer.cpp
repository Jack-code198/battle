#include "renderer.h"
#include "game_logic.h"
#include "ui.h"
#include <d3dx9.h>

extern LPDIRECT3DTEXTURE9 texBgCity1;
IDirect3D9* g_pD3D = NULL;
IDirect3DDevice9* g_pD3DDevice = NULL;
D3DPRESENT_PARAMETERS d3dpp;
LPD3DXSPRITE spriteBrush = NULL;

static bool g_IsBorderlessFullscreen = false;
static RECT g_WindowedRect = {};
static LONG g_WindowedStyle = 0;
static LONG g_WindowedExStyle = 0;

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

bool InitD3D() {
    g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (g_pD3D == NULL) return false;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    HRESULT hr = g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, g_hWnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &g_pD3DDevice);
    return SUCCEEDED(hr);
}

void Render() {
    if (g_pD3DDevice == NULL) return;
    g_pD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

    if (SUCCEEDED(g_pD3DDevice->BeginScene())) {
        if (spriteBrush != NULL) {
            spriteBrush->Begin(D3DXSPRITE_ALPHABLEND);

            if (texBgCity1) {
                D3DSURFACE_DESC desc;
                texBgCity1->GetLevelDesc(0, &desc);
                float scaleX = (float)SCREEN_WIDTH / (float)desc.Width;
                float scaleY = (float)SCREEN_HEIGHT / (float)desc.Height;

                D3DXMATRIX matScale;
                D3DXMatrixScaling(&matScale, scaleX, scaleY, 1.0f);
                spriteBrush->SetTransform(&matScale);

                D3DXVECTOR3 bgPos(0.0f, 0.0f, 0.0f);
                spriteBrush->Draw(texBgCity1, NULL, NULL, &bgPos, D3DCOLOR_XRGB(255, 255, 255));

                D3DXMATRIX matIdentity;
                D3DXMatrixIdentity(&matIdentity);
                spriteBrush->SetTransform(&matIdentity);
            }

            if (g_Player1.IsSuperMoveActive()) {
                D3DXVECTOR3 zeroPos(0, 0, 0);
                spriteBrush->Draw(NULL, NULL, NULL, &zeroPos, g_Player1.GetOverlayColor());
            }

            if (g_Player1.GetPosition().x < g_Player2.GetPosition().x) {
                g_Player1.Render(spriteBrush);
                g_Player2.Render(spriteBrush);
            }
            else {
                g_Player2.Render(spriteBrush);
                g_Player1.Render(spriteBrush);
            }

            if (g_ShowDebugHitboxes) {
                g_Player1.RenderDebugHitbox(spriteBrush);
                g_Player2.RenderDebugHitbox(spriteBrush);
            }

            spriteBrush->End();

            DrawBattleHud(
                spriteBrush,
                g_Player1.GetHealth(),
                g_Player1.GetMaxHealth(),
                g_Player1.GetSp(),
                g_Player1.GetMaxSp(),
                g_Player2.GetHealth(),
                g_Player2.GetMaxHealth(),
                g_Player2.GetSp(),
                g_Player2.GetMaxSp());
        }
        g_pD3DDevice->EndScene();
    }
    g_pD3DDevice->Present(NULL, NULL, NULL, NULL);
}

void CleanUpD3D() {
    CleanUpMakotoTextures();
    CleanUpJokerTextures();
    CleanUpHudTextures();

    if (texBgCity1) { texBgCity1->Release(); texBgCity1 = NULL; }

    if (spriteBrush != NULL) { spriteBrush->Release(); spriteBrush = NULL; }
    if (g_pD3DDevice != NULL) { g_pD3DDevice->Release(); g_pD3DDevice = NULL; }
    if (g_pD3D != NULL) { g_pD3D->Release(); g_pD3D = NULL; }
}

void ToggleFullscreen() {
    if (g_pD3DDevice == NULL || g_hWnd == NULL) return;

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

bool IsFullscreen() {
    return g_IsBorderlessFullscreen;
}
