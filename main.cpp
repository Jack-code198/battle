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

// Melee hitboxes use named constants from config.h (no magic brace lists).
AttackData attackHitbox = {
    ATK_NEUTRAL_START, ATK_NEUTRAL_END, ATK_NEUTRAL_DAMAGE,
    ATK_NEUTRAL_OFFSET_X, ATK_NEUTRAL_OFFSET_Y, ATK_NEUTRAL_WIDTH, ATK_NEUTRAL_HEIGHT
};
AttackData sideAttackHitbox = {
    ATK_SIDE_START, ATK_SIDE_END, ATK_SIDE_DAMAGE,
    ATK_SIDE_OFFSET_X, ATK_SIDE_OFFSET_Y, ATK_SIDE_WIDTH, ATK_SIDE_HEIGHT
};
AttackData attackUpHitbox = {
    ATK_UP_START, ATK_UP_END, ATK_UP_DAMAGE,
    ATK_UP_OFFSET_X, ATK_UP_OFFSET_Y, ATK_UP_WIDTH, ATK_UP_HEIGHT
};
AttackData downAttackHitbox = {
    ATK_DOWN_START, ATK_DOWN_END, ATK_DOWN_DAMAGE,
    ATK_DOWN_OFFSET_X, ATK_DOWN_OFFSET_Y, ATK_DOWN_WIDTH, ATK_DOWN_HEIGHT
};

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
            SetMenuCursorEnabled(true);
            RenderMenuScreen();

            if (menuChoice == 1) { // Start Game
                g_CurrentState = GameState::StageSelect;
                ResetStageSelectInputState(); // don't let the same Enter press confirm a stage too
            }
            menuChoice = 0; // consume the confirmation so it doesn't re-trigger
            break;
        }
        case GameState::StageSelect: {
            SetMenuCursorEnabled(true);
            RenderStageSelectScreen();

            if (stageChoice == 1) { // confirmed
                ApplySelectedStageToBattle();
                g_SoundManager.StopMenuMusic();
                g_SoundManager.PlayBattleMusic();
                SetMenuCursorEnabled(false);
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
            SetMenuCursorEnabled(false);
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