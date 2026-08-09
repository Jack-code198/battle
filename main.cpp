#include <stdio.h>
#include <cstring>
#include "config.h"
#include "input.h"
#include "game_logic.h"
#include "renderer.h"
#include "player/makoto/Makoto.h"
#include "player/joker/Joker.h"
#include "audio.h"
#include "ui.h"

const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 768;

LPDIRECT3DTEXTURE9 texBgCity1 = NULL;

SpriteSheetBounds g_MessiahSheetBounds = { 0.0f, 0.0f };
SpriteSheetBounds g_MegidolaonBurstBounds = { 0.0f, 0.0f };
SpriteSheetBounds g_MegidolaonBlastBounds = { 0.0f, 0.0f };

AttackData attackHitbox = { 3, 10, 28, 18, -18, 28, 40 };
AttackData sideAttackHitbox = { 2, 8, 32, 28, -22, 36, 42 };
AttackData attackUpHitbox = { 2, 5, 32, 22, -22, 32, 48 };
AttackData downAttackHitbox = { 2, 8, 30, 14, -36, 40, 32 };

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    CreateMyWindow();
    CreateDirectInput();

    if (!InitD3D()) return 0;

    D3DXCreateSprite(g_pD3DDevice, &spriteBrush);

    if (!LoadMakotoTextures()) {
        return 0;
    }

    if (!LoadJokerTextures()) {
        MessageBox(g_hWnd, "Failed to load assets/joker textures", "Error", MB_OK);
    }

    LoadHudTextures();
    ResetBattleHud(g_Player1.GetMaxHealth(), g_Player2.GetMaxHealth());

    HRESULT hr = D3DXCreateTextureFromFileEx(
        g_pD3DDevice,
        "assets/background/City3/city3.png",
        D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT,
        NULL, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT,
        0,
        NULL, NULL, &texBgCity1);

    if (FAILED(hr)) {
        MessageBox(g_hWnd, "Failed to load assets/background/City3/city3.png", "Error", MB_OK);
    }

    if (!g_SoundManager.Initialise()) {
        MessageBox(g_hWnd, "FMOD init failed. See FMOD/README.txt", "Error", MB_OK);
    }
    else {
        g_SoundManager.PlayBattleMusic();
    }

    g_GameTimer.Init(GAME_ANIMATION_FPS);

    while (WindowIsRunning()) {
        DWORD frameStart = GetTickCount();
        GetInput();
        g_SoundManager.Update();
        g_Player2.Update();
        g_Player1.Update();
        Render();

        DWORD frameElapsed = GetTickCount() - frameStart;
        if (frameElapsed < 8) {
            Sleep(8 - frameElapsed);
        }
    }

    g_SoundManager.Shutdown();
    CleanUpD3D();
    CleanUpDirectInput();
    CleanUpWindow();

    return (int)msg.wParam;
}
