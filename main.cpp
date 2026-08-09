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
#include "menuMain.h"
#include "stageSelect.h"
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

// Game State
// DEBUG: player selection isn't wired up yet, so confirming a stage jumps
// straight into the fight with the current fighters.
enum class GameState {
    MainMenu,
    StageSelect,
    Battle
};

static GameState g_CurrentState = GameState::MainMenu;
int menuChoice = 0;
static int stageChoice = 0;

static void RenderMenuScreen() {
    if (!g_pD3DDevice) return;

    g_pD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

    if (SUCCEEDED(g_pD3DDevice->BeginScene())) {
        mainMenu(menuChoice); // updates selection/input AND draws the menu sprites
        g_pD3DDevice->EndScene();
    }

    g_pD3DDevice->Present(NULL, NULL, NULL, NULL);
}

static void RenderStageSelectScreen() {
    if (!g_pD3DDevice) return;

    g_pD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

    if (SUCCEEDED(g_pD3DDevice->BeginScene())) {
        stageSelectScreen(stageChoice); // updates selection/input AND draws the preview
        g_pD3DDevice->EndScene();
    }

    g_pD3DDevice->Present(NULL, NULL, NULL, NULL);
}

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

    if (!LoadMenuTextures()) {
        MessageBox(g_hWnd, "Main menu assets failed to load - check assets/background/MainMenu.jpg and assets/font", "Warning", MB_OK);
    }

    if (!LoadStageTextures()) {
        MessageBox(g_hWnd, "One or more stage textures failed to load - check stageSelect.cpp paths", "Warning", MB_OK);
    }
    ApplySelectedStageToBattle(); // give texBgCity1 a valid default before Battle is ever reached


    if (!g_SoundManager.Initialise()) {
        MessageBox(g_hWnd, "FMOD init failed. See FMOD/README.txt", "Error", MB_OK);
    }
    else {
        g_SoundManager.PlayMenuMusic();
    }

    g_GameTimer.Init(GAME_ANIMATION_FPS);

    while (WindowIsRunning()) {
        DWORD frameStart = GetTickCount();
        GetInput();
        g_SoundManager.Update();

        //main menu -> stage select -> battle -> victory screen -> main menu
        // DEBUG: player selection and the victory screen aren't wired up yet.
        switch (g_CurrentState) {
        case GameState::MainMenu: {
            RenderMenuScreen();

            if (menuChoice == 1) { // Start Game
                g_CurrentState = GameState::StageSelect;
                ResetStageSelectInputState(); // don't let the same Enter press confirm a stage too
            }
            menuChoice = 0; // consume the confirmation so it doesn't re-trigger
            break;
        }
        case GameState::StageSelect: {
            RenderStageSelectScreen();

            if (stageChoice == 1) { // confirmed
                ApplySelectedStageToBattle();
                g_SoundManager.StopMenuMusic();
                g_SoundManager.PlayBattleMusic();
                g_CurrentState = GameState::Battle;
            }
            else if (stageChoice == 2) { // back
                g_CurrentState = GameState::MainMenu;
                ResetMenuInputState(); // don't let the same key press re-trigger a menu option
            }
            stageChoice = 0; // consume so it doesn't re-trigger next frame
            break;
        }
        case GameState::Battle: {
            g_Player2.Update();
            g_Player1.Update();
            Render();
            break;
        }
        }

        DWORD frameElapsed = GetTickCount() - frameStart;
        if (frameElapsed < 8) {
            Sleep(8 - frameElapsed);
        }
    }

    CleanUpMenuTextures();
    CleanUpStageTextures();
    g_SoundManager.Shutdown();
    CleanUpD3D();
    CleanUpDirectInput();
    CleanUpWindow();

    return (int)msg.wParam;
}