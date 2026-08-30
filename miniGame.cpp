#include "MiniGame.h"
#include "Input.h"
#include "Audio.h"
#include "Ball.h"

static ID3DXFont* g_MiniGameTitleFont = nullptr;
static ID3DXFont* g_MiniGameHintFont = nullptr;
static bool g_MiniGameEscHeld = false;

static LPDIRECT3DTEXTURE9 g_MiniBall1Texture = nullptr;
static LPDIRECT3DTEXTURE9 g_MiniBall2Texture = nullptr;
static Ball g_MiniBall1(BALL_1_MASS);
static Ball g_MiniBall2(BALL_2_MASS);

static bool CreateMiniGameFont(const char* familyName, INT height, ID3DXFont** outFont) {
    HRESULT hr = D3DXCreateFontA(
        g_pD3DDevice,
        height,
        0,
        FW_BOLD,
        1,
        FALSE,
        DEFAULT_CHARSET,
        OUT_TT_PRECIS,
        ANTIALIASED_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        familyName,
        outFont);
    return SUCCEEDED(hr) && *outFont != nullptr;
}

static bool EnsureMiniGameFonts() {
    if (g_MiniGameTitleFont && g_MiniGameHintFont) {
        return true;
    }

    if (!CreateMiniGameFont(NORMAL_FONT_FAMILY, 42, &g_MiniGameTitleFont)) {
        CreateMiniGameFont("Arial", 42, &g_MiniGameTitleFont);
    }
    if (!CreateMiniGameFont(NORMAL_FONT_FAMILY, 14, &g_MiniGameHintFont)) {
        CreateMiniGameFont("Arial", 14, &g_MiniGameHintFont);
    }

    return g_MiniGameTitleFont && g_MiniGameHintFont;
}

bool LoadMiniGameAssets() {
    EnsureMiniGameFonts();
    LoadBallTexture(&g_MiniBall1Texture, BALL1_ICON_PATH);
    LoadBallTexture(&g_MiniBall2Texture, BALL2_ICON_PATH);
    g_MiniBall1.SetTexture(g_MiniBall1Texture);
    g_MiniBall2.SetTexture(g_MiniBall2Texture);
    return true;
}

void CleanUpMiniGameAssets() {
    if (g_MiniGameTitleFont) {
        g_MiniGameTitleFont->Release();
        g_MiniGameTitleFont = nullptr;
    }
    if (g_MiniGameHintFont) {
        g_MiniGameHintFont->Release();
        g_MiniGameHintFont = nullptr;
    }
    CleanUpBallTexture(g_MiniBall1Texture);
    CleanUpBallTexture(g_MiniBall2Texture);
}

void ResetMiniGameState() {
    g_MiniBall1.Reset((float)SCREEN_WIDTH * 0.32f, (float)SCREEN_HEIGHT * 0.55f);
    g_MiniBall2.Reset((float)SCREEN_WIDTH * 0.68f, (float)SCREEN_HEIGHT * 0.55f);
    g_MiniGameEscHeld = IsUiKeyDown(DIK_ESCAPE);
}

static void UpdateMiniGameBalls() {
    g_MiniBall1.BeginFrame();
    g_MiniBall2.BeginFrame();

    g_MiniBall1.SetThrustActive(IsUiKeyDown(DIK_UP));
    g_MiniBall1.SetRotateLeft(IsUiKeyDown(DIK_LEFT));
    g_MiniBall1.SetRotateRight(IsUiKeyDown(DIK_RIGHT));

    g_MiniBall2.SetThrustActive(IsUiKeyDown(DIK_W));
    g_MiniBall2.SetRotateLeft(IsUiKeyDown(DIK_A));
    g_MiniBall2.SetRotateRight(IsUiKeyDown(DIK_D));

    if (g_MiniBall1.IntegrateStep()) {
        LogCollisionDetected("ball wall");
        g_SoundManager.PlayBallCollisionSfx(g_MiniBall1.GetCenterX());
    }
    if (g_MiniBall2.IntegrateStep()) {
        LogCollisionDetected("ball wall");
        g_SoundManager.PlayBallCollisionSfx(g_MiniBall2.GetCenterX());
    }

    float pairCollisionX = 0.0f;
    if (Ball::ResolvePairCollision(g_MiniBall1, g_MiniBall2, &pairCollisionX)) {
        LogCollisionDetected("ball vs ball");
        g_SoundManager.PlayBallCollisionSfx(pairCollisionX);
    }
}

static void RenderMiniGameFrame() {
    if (!spriteBrush) {
        return;
    }

    spriteBrush->Begin(D3DXSPRITE_ALPHABLEND);
    DrawDebugRect(
        spriteBrush,
        0.0f,
        0.0f,
        (float)SCREEN_WIDTH,
        (float)SCREEN_HEIGHT,
        D3DCOLOR_ARGB(255, 0, 0, 0));

    g_MiniBall1.Render(spriteBrush);
    g_MiniBall2.Render(spriteBrush);

    if (g_MiniGameTitleFont) {
        RECT titleRect = { 0, 32, SCREEN_WIDTH, 88 };
        g_MiniGameTitleFont->DrawTextA(
            spriteBrush,
            "MINI GAME",
            -1,
            &titleRect,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE,
            D3DCOLOR_XRGB(255, 230, 80));
    }

    if (g_MiniGameHintFont) {
        RECT hintRect = { 48, SCREEN_HEIGHT - 56, SCREEN_WIDTH - 48, SCREEN_HEIGHT - 12 };
        g_MiniGameHintFont->DrawTextA(
            spriteBrush,
            "Ball 1: Arrows (Up thrust, Left/Right rotate)   Ball 2: WASD (W thrust, A/D rotate)\r\n"
            "Collision SFX pans by ball X (left = left ear). Esc: main menu",
            -1,
            &hintRect,
            DT_CENTER | DT_BOTTOM,
            D3DCOLOR_XRGB(180, 180, 180));
    }

    spriteBrush->End();
}

void miniGameScreen(int& choice) {
    if (!EnsureMiniGameFonts()) {
        return;
    }

    const bool escPressed = IsUiKeyDown(DIK_ESCAPE);
    if (escPressed && !g_MiniGameEscHeld) {
        g_SoundManager.PlaySelectionSound();
        choice = 2;
    }
    g_MiniGameEscHeld = escPressed;

    if (choice == 2) {
        return;
    }

    UpdateMiniGameBalls();
    RenderMiniGameFrame();
}
