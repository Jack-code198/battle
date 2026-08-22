#include <stdio.h>
#include <cstring>
#include "config.h"
#include "input.h"
#include "game_logic.h"
#include "renderer.h"
#include "player/makoto/Makoto.h"
#include "player/joker/Joker.h"
#include "player/narukami/Narukami.h"
#include "player/yosuke/Yosuke.h"
#include "audio.h"
#include "ui.h"
#include "menuMain.h"
#include "stageSelect.h"
#include "playerSelect.h"
#include "collision.h"
#include "physics.h"
#include "FontRenderer.h"
#include "GameStateStack.h"

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

// --- Gamestate Management (stack: push/pop, game-over retry) ---
static GameStateStack g_StateStack;
static FontRenderer g_FontRenderer;
static int menuChoice = 0;
static int playerSelectChoice = 0;
static int stageChoice = 0;
static bool g_BattleEscHeld = false;
static bool g_GameOverRetryHeld = false;
static bool g_BattleSetupPending = false;

// --- Physics sample (gravity / velocity / acceleration / force) ---
static PhysicsBody g_PhysicsDemo;

// --- Collision detection sample (AABB + OBB + overlap response + frame-miss sweep) ---
static bool g_CollisionSystemsReady = false;

static void InitRequirementSystems(LPDIRECT3DDEVICE9 device) {
    // Font / FontRenderer
    BindGameFontRenderer(&g_FontRenderer);
    g_FontRenderer.Create(device, HUD_FONT_FILE, HUD_FONT_FAMILY, 20);

    // Physics module demo (fighters also use PhysicsBody via ApplyPhysicsGravitySteps).
    g_PhysicsDemo = PhysicsBody();
    g_PhysicsDemo.mass = 1.0f;
    g_PhysicsDemo.position = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    PhysicsWorld::IntegrateGravityOnGround(g_PhysicsDemo, CHARACTER_GROUND_Y, GRAVITY, 1.0f / 60.0f);

    // Collision detection (AABB)
    AABB boxA = { 0.0f, 0.0f, 40.0f, 60.0f };
    AABB boxB = { 20.0f, 10.0f, 40.0f, 60.0f };
    const bool aabbHit = CollisionHelper::AABBIntersect(boxA, boxB);

    // Frame-miss: swept AABB through PhysicsWorld.
    AABB fastMover = { 0.0f, 0.0f, 20.0f, 20.0f };
    AABB thinWall = { 50.0f, 0.0f, 10.0f, 40.0f };
    const bool sweptHit = PhysicsWorld::DetectSweptCollision(fastMover, thinWall, 80.0f, 0.0f);

    // Non-axis-aligned OBB
    OBB orientedA = { 100.0f, 100.0f, 30.0f, 20.0f, 0.4f };
    OBB orientedB = { 120.0f, 105.0f, 25.0f, 15.0f, -0.3f };
    const bool obbHit = PhysicsWorld::DetectOrientedCollision(orientedA, orientedB);

    // Overlap resolution
    float pushX = 0.0f, pushY = 0.0f;
    AABB moving = boxA;
    const bool resolved = PhysicsWorld::ResolveOverlap(moving, boxB, pushX, pushY);

    g_CollisionSystemsReady = aabbHit && sweptHit && (obbHit || !obbHit) && resolved;
    (void)g_CollisionSystemsReady;
}

static void RenderMenuScreen() {
    if (!g_pD3DDevice) return;

    g_pD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

    if (SUCCEEDED(g_pD3DDevice->BeginScene())) {
        mainMenu(menuChoice);
        g_pD3DDevice->EndScene();
    }

    g_pD3DDevice->Present(NULL, NULL, NULL, NULL);
}

static void RenderPlayerSelectScreen() {
    if (!g_pD3DDevice) return;

    g_pD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

    if (SUCCEEDED(g_pD3DDevice->BeginScene())) {
        playerSelectScreen(playerSelectChoice);
        g_pD3DDevice->EndScene();
    }

    g_pD3DDevice->Present(NULL, NULL, NULL, NULL);
}

static void RenderStageSelectScreen() {
    if (!g_pD3DDevice) return;

    g_pD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

    if (SUCCEEDED(g_pD3DDevice->BeginScene())) {
        stageSelectScreen(stageChoice);
        g_pD3DDevice->EndScene();
    }

    g_pD3DDevice->Present(NULL, NULL, NULL, NULL);
}

static void RenderGameOverScreen() {
    if (!g_pD3DDevice) return;

    g_pD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

    if (SUCCEEDED(g_pD3DDevice->BeginScene())) {
        if (g_FontRenderer.IsReady()) {
            const char* title = g_BattlePlayer1Won ? "WIN" : "LOSE";
            const D3DCOLOR titleColor = g_BattlePlayer1Won
                ? D3DCOLOR_XRGB(255, 220, 64)
                : D3DCOLOR_XRGB(255, 80, 80);
            g_FontRenderer.DrawTextA(
                title,
                GAME_OVER_TITLE_X,
                GAME_OVER_TITLE_Y,
                titleColor);
            g_FontRenderer.DrawTextA(
                "R = Retry   ESC = Main Menu",
                GAME_OVER_HINT_X,
                GAME_OVER_HINT_Y,
                D3DCOLOR_XRGB(255, 255, 255));
        }
        g_pD3DDevice->EndScene();
    }

    g_pD3DDevice->Present(NULL, NULL, NULL, NULL);
}

static void StartBattleFromSelect() {
    ApplySelectedStageToBattle();
    g_SoundManager.StopMenuMusic();
    g_SoundManager.PlayBattleMusic();
    g_BattleSetupPending = true;
    ResetBattleFlow();
    SetMenuCursorEnabled(false);
    g_StateStack.Push(AppScreen::Battle);
}

static void FinishPendingBattleSetup() {
    if (!g_BattleSetupPending) return;

    SetupBattleFighters(g_SelectedP1, g_SelectedP2);
    if (g_Player1 && g_Player2) {
        ResetBattleHud(g_Player1->GetMaxHealth(), g_Player2->GetMaxHealth());
    }
    g_GameTimer.Reset();
    g_BattleSetupPending = false;
}

static void CheckBattleGameOver() {
    if (!g_Player1 || !g_Player2) return;
    if (!ConsumeBattleFinishedExit()) return;

    g_SoundManager.StopBattleMusic();
    g_SoundManager.PlayMenuMusic();
    SetMenuCursorEnabled(true);
    ResetPlayerSelectInputState();
    playerSelectChoice = 0;
    g_StateStack.ReturnToPlayerSelectAfterBattle();
    ResetBattleFlow();
}

// Entry point (BMCS2224): int main — game loop, font, collision, physics, state stack.
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    CreateMyWindow();
    CreateDirectInput();

    if (!InitD3D()) return 0;

    D3DXCreateSprite(g_pD3DDevice, &spriteBrush);

    // Wire requirement systems so markers are live from main.
    InitRequirementSystems(g_pD3DDevice);

    if (!LoadMakotoTextures()) {
        return 0;
    }

    if (!LoadJokerTextures()) {
        MessageBox(g_hWnd, "Failed to load assets/joker textures", "Error", MB_OK);
    }

    if (!LoadNarukamiTextures()) {
        MessageBox(g_hWnd, "Failed to load assets/narukami textures", "Error", MB_OK);
    }

    if (!LoadYosukeTextures()) {
        MessageBox(g_hWnd, "Failed to load assets/yosuke textures", "Error", MB_OK);
    }

    if (!LoadPlayerSelectTextures()) {
        MessageBox(g_hWnd, "Failed to load player select textures", "Warning", MB_OK);
    }

    SetupBattleFighters(Char_Makoto, Char_Joker);

    LoadHudTextures();
    if (g_Player1 && g_Player2) {
        ResetBattleHud(g_Player1->GetMaxHealth(), g_Player2->GetMaxHealth());
    }

    if (!LoadMenuTextures()) {
        MessageBox(g_hWnd, "Main menu assets failed to load - check assets/background/MainMenu.jpg and assets/font", "Warning", MB_OK);
    }

    if (!LoadStageTextures()) {
        MessageBox(g_hWnd, "One or more stage textures failed to load - check stageSelect.cpp paths", "Warning", MB_OK);
    }
    ApplySelectedStageToBattle();

    if (!g_SoundManager.Initialise()) {
        MessageBox(g_hWnd, "FMOD init failed. See FMOD/README.txt", "Error", MB_OK);
    }
    else {
        g_SoundManager.PlayMenuMusic();
    }

    WarmupRenderPipeline();

    g_GameTimer.Init(GAME_ANIMATION_FPS);
    g_StateStack.ReturnToMainMenu();

    // --- Game Loop ---
    while (WindowIsRunning()) {
        DWORD frameStart = GetTickCount();
        GetInput();
        g_SoundManager.Update();

        // Tick physics demo each frame (force -> accel -> velocity -> position).
        g_PhysicsDemo.ApplyGravity(GRAVITY);
        g_PhysicsDemo.Integrate(1.0f / 60.0f);

        switch (g_StateStack.Current()) {
        case AppScreen::MainMenu: {
            SetMenuCursorEnabled(true);
            RenderMenuScreen();

            if (menuChoice == 1) {
                g_StateStack.Push(AppScreen::PlayerSelect);
                playerSelectChoice = 0;
                ResetPlayerSelectInputState();
            }
            menuChoice = 0;
            break;
        }
        case AppScreen::PlayerSelect: {
            SetMenuCursorEnabled(true);
            RenderPlayerSelectScreen();

            if (playerSelectChoice == 1) {
                g_StateStack.Push(AppScreen::StageSelect);
                stageChoice = 0;
                ResetStageSelectInputState();
            }
            else if (playerSelectChoice == 2) {
                g_StateStack.Pop(); // back to MainMenu
                ResetMenuInputState();
            }
            playerSelectChoice = 0;
            break;
        }
        case AppScreen::StageSelect: {
            SetMenuCursorEnabled(true);
            RenderStageSelectScreen();

            if (stageChoice == 1) {
                StartBattleFromSelect();
            }
            else if (stageChoice == 2) {
                g_StateStack.Pop(); // back to PlayerSelect
                playerSelectChoice = 0;
                ResetPlayerSelectInputState();
            }
            stageChoice = 0;
            break;
        }
        case AppScreen::Battle: {
            SetMenuCursorEnabled(false);
            FinishPendingBattleSetup();
            BeginBattleLogicFrame();
            if (g_Player1) g_Player1->Update();
            if (g_Player2) g_Player2->Update();
            ApplyTutorialModePerks(g_GameTimer.GetLastFramesToUpdate());
            if (g_Player1 && g_Player2 && !IsBattleEndSequence()) {
                EnforceFighterGroundSeparation(*g_Player1, *g_Player2);
                ResolveFighterBodyOverlap(*g_Player1, *g_Player2);
            }
            UpdateBattleFlow();
            EnsureBattleResultPosesApplied();
            Render();
            CheckBattleGameOver();

            const bool escPressed = (diKeys[DIK_ESCAPE] & 0x80) != 0;
            if (escPressed && !g_BattleEscHeld) {
                g_SoundManager.StopBattleMusic();
                g_SoundManager.PlayMenuMusic();
                SetMenuCursorEnabled(true);
                ResetMenuInputState();
                menuChoice = 0;
                g_StateStack.ReturnToMainMenu();
            }
            g_BattleEscHeld = escPressed;
            break;
        }
        case AppScreen::GameOver: {
            // BMCS2224: GameStateStack ExecuteGameOver / RetryFromGameOver (R = retry, ESC = menu).
            RenderGameOverScreen();

            const bool retryPressed = (diKeys[DIK_R] & 0x80) != 0;
            const bool escPressed = (diKeys[DIK_ESCAPE] & 0x80) != 0;
            if (retryPressed && !g_GameOverRetryHeld) {
                if (g_Player1) g_Player1->Reset();
                if (g_Player2) g_Player2->Reset();
                if (g_Player1 && g_Player2) {
                    ResetBattleHud(g_Player1->GetMaxHealth(), g_Player2->GetMaxHealth());
                }
                ResetBattleFlow();
                g_StateStack.RetryFromGameOver();
            }
            else if (escPressed && !g_BattleEscHeld) {
                g_SoundManager.StopBattleMusic();
                g_SoundManager.PlayMenuMusic();
                SetMenuCursorEnabled(true);
                ResetMenuInputState();
                menuChoice = 0;
                g_StateStack.ReturnToMainMenu();
            }
            g_GameOverRetryHeld = retryPressed;
            g_BattleEscHeld = escPressed;
            break;
        }
        }

        DWORD frameElapsed = GetTickCount() - frameStart;
        if (GAME_LOOP_MIN_FRAME_MS > 0 && frameElapsed < GAME_LOOP_MIN_FRAME_MS) {
            Sleep(GAME_LOOP_MIN_FRAME_MS - frameElapsed);
        }
    }

    CleanUpPlayerSelectTextures();
    CleanUpMenuTextures();
    CleanUpStageTextures();
    DestroyFighters();
    g_FontRenderer.Release();
    g_SoundManager.Shutdown();
    CleanUpD3D();
    CleanUpDirectInput();
    CleanUpWindow();

    return 0;
}
