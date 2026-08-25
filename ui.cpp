#include "ui.h"
#include "tutorial_guide.h"
#include "renderer.h"
#include "game_logic.h"
#include "player/CharacterId.h"
#include <algorithm>

static LPDIRECT3DTEXTURE9 g_MakotoIconTex = nullptr;
static LPDIRECT3DTEXTURE9 g_JokerIconTex = nullptr;
static LPDIRECT3DTEXTURE9 g_NarukamiIconTex = nullptr;
static LPDIRECT3DTEXTURE9 g_YosukeIconTex = nullptr;
static ID3DXFont* g_HudFont = nullptr;
static ID3DXFont* g_BannerFont = nullptr;
static ID3DXFont* g_TutorialFont = nullptr;
static bool g_HudFontLoaded = false;
static bool g_TutorialFontLoaded = false;
static const char* g_LoadedHudFontPath = nullptr;

struct HudHealthTracker {
    float displayHealth = 0.0f; // delayed chip visual (Tekken-style)
    int syncedHealth = -1;      // last known actual HP
    int chipHoldFrames = 0;     // wait after hit before chip drains
};

static HudHealthTracker g_P1HealthTrack;
static HudHealthTracker g_P2HealthTrack;

enum HudIconColorKey {
    HUD_ICON_NO_COLORKEY = 0,
    HUD_ICON_PERSONA_BLUE,
    HUD_ICON_JOKER_RED,
    HUD_ICON_NARUKAMI_YELLOW,
    HUD_ICON_YOSUKE_YELLOW
};

static bool LoadHudFont() {
    if (g_HudFontLoaded) {
        return g_HudFont != nullptr;
    }

    if (AddFontResourceExA(NORMAL_FONT_FILE, FR_PRIVATE, 0) != 0) {
        g_LoadedHudFontPath = NORMAL_FONT_FILE;
    }

    g_HudFontLoaded = true;

    auto tryCreateFont = [](const char* familyName) -> HRESULT {
        return D3DXCreateFontA(
            g_pD3DDevice,
            20,
            0,
            FW_BOLD,
            1,
            FALSE,
            DEFAULT_CHARSET,
            OUT_TT_PRECIS,
            ANTIALIASED_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            familyName,
            &g_HudFont);
    };

    HRESULT hr = tryCreateFont(NORMAL_FONT_FAMILY);
    if (FAILED(hr) || !g_HudFont) {
        if (g_HudFont) {
            g_HudFont->Release();
            g_HudFont = nullptr;
        }
        hr = tryCreateFont("Arial");
    }

    return SUCCEEDED(hr) && g_HudFont != nullptr;
}

static bool LoadTutorialFont() {
    if (g_TutorialFontLoaded) {
        return g_TutorialFont != nullptr;
    }

    if (AddFontResourceExA(NORMAL_FONT_FILE, FR_PRIVATE, 0) != 0 && !g_LoadedHudFontPath) {
        g_LoadedHudFontPath = NORMAL_FONT_FILE;
    }

    g_TutorialFontLoaded = true;
    if (!g_pD3DDevice) return false;

    auto tryCreateFont = [](const char* familyName, INT height) -> HRESULT {
        if (g_TutorialFont) {
            g_TutorialFont->Release();
            g_TutorialFont = nullptr;
        }
        return D3DXCreateFontA(
            g_pD3DDevice,
            height,
            0,
            FW_NORMAL,
            1,
            FALSE,
            DEFAULT_CHARSET,
            OUT_TT_PRECIS,
            ANTIALIASED_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            familyName,
            &g_TutorialFont);
    };

    static const char* kTutorialFontCandidates[] = {
        NORMAL_FONT_FAMILY,
        "Arial"
    };

    static const INT kTutorialFontHeight = 17;

    HRESULT hr = E_FAIL;
    for (const char* familyName : kTutorialFontCandidates) {
        hr = tryCreateFont(familyName, kTutorialFontHeight);
        if (SUCCEEDED(hr) && g_TutorialFont) break;
    }

    return SUCCEEDED(hr) && g_TutorialFont != nullptr;
}

static void EnsureBannerFont() {
    if (g_BannerFont || !g_pD3DDevice) return;
    D3DXCreateFontA(
        g_pD3DDevice,
        120,
        0,
        FW_BOLD,
        1,
        FALSE,
        DEFAULT_CHARSET,
        OUT_TT_PRECIS,
        ANTIALIASED_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        HUD_FONT_FAMILY,
        &g_BannerFont);
}

static void DrawCenteredBanner(const char* text, D3DCOLOR color) {
    if (!text) return;
    EnsureBannerFont();
    if (!g_BannerFont) return;

    RECT rect;
    rect.left = 0;
    rect.top = (LONG)(SCREEN_HEIGHT * 0.35f);
    rect.right = SCREEN_WIDTH;
    rect.bottom = rect.top + 160;
    g_BannerFont->DrawTextA(nullptr, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_NOCLIP, color);
}

void DrawBattleRoundOverlay() {
    if (ShouldShowBattleKo()) {
        DrawCenteredBanner("KO", D3DCOLOR_XRGB(255, 48, 48));
        return;
    }
    if (const char* label = GetBattleCountdownLabel()) {
        const D3DCOLOR color = (label[0] == 'F')
            ? D3DCOLOR_XRGB(255, 220, 64)
            : D3DCOLOR_XRGB(255, 255, 255);
        DrawCenteredBanner(label, color);
    }
}

void DrawBattleTimerOverlay() {
    if (!IsBattleCombatActive() || IsTutorialBattleMode() || IsBattleEndSequence()) return;
    if (!LoadHudFont() || !g_HudFont) return;

    const int seconds = (g_BattleTimeRemainingSteps + 59) / 60;
    char timeText[16];
    sprintf_s(timeText, "%02d", seconds);

    const D3DCOLOR color = (seconds <= 10)
        ? D3DCOLOR_XRGB(255, 72, 72)
        : D3DCOLOR_XRGB(255, 255, 255);

    RECT rect = {
        SCREEN_WIDTH / 2 - 48,
        6,
        SCREEN_WIDTH / 2 + 48,
        42
    };
    g_HudFont->DrawTextA(
        nullptr,
        timeText,
        -1,
        &rect,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE,
        color);
}

void DrawHitComboOverlay() {
    if (!IsBattleCombatActive() || IsTutorialBattleMode() || IsBattleEndSequence()) return;
    if (!LoadHudFont() || !g_HudFont) return;

    const int combo = GetP1HitCombo();
    if (combo < HIT_COMBO_MIN_DISPLAY) return;

    char comboText[32];
    sprintf_s(comboText, "%d HIT COMBO!", combo);

    const float iconSize = HUD_ICON_SIZE;
    const float barGap = HUD_BAR_GAP;
    const float totalBarStackHeight =
        HUD_HP_BAR_HEIGHT + HUD_SP_BAR_HEIGHT + HUD_STAMINA_BAR_HEIGHT + barGap * 2.0f;
    const float p1IconX = HUD_EDGE_MARGIN;
    const float p1BarX = p1IconX + iconSize + HUD_ICON_BAR_GAP;
    const float p1BarY = HUD_TOP_Y + (iconSize - totalBarStackHeight) * 0.5f;
    const float comboY = p1BarY + totalBarStackHeight + HUD_COMBO_OFFSET_Y;

    RECT rect = {
        (LONG)p1BarX,
        (LONG)comboY,
        (LONG)(p1BarX + HUD_BAR_WIDTH),
        (LONG)(comboY + HUD_NAME_TEXT_HEIGHT)
    };
    g_HudFont->DrawTextA(
        nullptr,
        comboText,
        -1,
        &rect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE,
        D3DCOLOR_XRGB(255, 220, 64));
}

void DrawBattleFadeOverlay(LPD3DXSPRITE sprite) {
    if (!sprite) return;

    int alpha = 0;
    if (g_BattleFlowPhase == BattleFlowPhase::FadeOut) {
        alpha = (int)(255.0f * (float)g_BattleFlowTimer / (float)BATTLE_FADE_OUT_STEPS);
    }
    else if (g_BattleFlowPhase == BattleFlowPhase::Finished) {
        alpha = 255;
    }
    else {
        return;
    }

    if (alpha < 0) alpha = 0;
    if (alpha > 255) alpha = 255;
    DrawDebugRect(
        sprite,
        0.0f,
        0.0f,
        (float)SCREEN_WIDTH,
        (float)SCREEN_HEIGHT,
        D3DCOLOR_ARGB((BYTE)alpha, 0, 0, 0));
}

static void DrawHudText(const char* text, float x, float y, D3DCOLOR color, bool rightAlign, float alignEdgeX) {
    if (!g_HudFont || !text) return;

    // RECT size from named HUD layout constants (not hardcoded per call).
    RECT rect;
    if (rightAlign) {
        rect.left = (LONG)(alignEdgeX - HUD_NAME_TEXT_WIDTH);
        rect.right = (LONG)alignEdgeX;
    }
    else {
        rect.left = (LONG)x;
        rect.right = (LONG)(x + HUD_NAME_TEXT_WIDTH);
    }
    rect.top = (LONG)y;
    rect.bottom = (LONG)(y + HUD_NAME_TEXT_HEIGHT);

    UINT format = DT_NOCLIP | (rightAlign ? DT_RIGHT : DT_LEFT);
    g_HudFont->DrawTextA(NULL, text, -1, &rect, format, color);
}

static bool LoadHudIconTexture(const char* path, LPDIRECT3DTEXTURE9* outTex, HudIconColorKey colorKey) {
    if (!outTex || !g_pD3DDevice) return false;

    D3DCOLOR keyColor = 0;
    if (colorKey == HUD_ICON_PERSONA_BLUE) {
        keyColor = D3DCOLOR_XRGB(PERSONA_COLORKEY_R, PERSONA_COLORKEY_G, PERSONA_COLORKEY_B);
    }
    else if (colorKey == HUD_ICON_JOKER_RED) {
        keyColor = D3DCOLOR_XRGB(JOKER_COLORKEY_R, JOKER_COLORKEY_G, JOKER_COLORKEY_B);
    }
    else if (colorKey == HUD_ICON_NARUKAMI_YELLOW) {
        keyColor = D3DCOLOR_XRGB(NARUKAMI_COLORKEY_R, NARUKAMI_COLORKEY_G, NARUKAMI_COLORKEY_B);
    }
    else if (colorKey == HUD_ICON_YOSUKE_YELLOW) {
        keyColor = D3DCOLOR_XRGB(YOSUKE_COLORKEY_R, YOSUKE_COLORKEY_G, YOSUKE_COLORKEY_B);
    }

    HRESULT hr = D3DXCreateTextureFromFileEx(
        g_pD3DDevice,
        path,
        D3DX_DEFAULT_NONPOW2,
        D3DX_DEFAULT_NONPOW2,
        D3DX_DEFAULT,
        NULL,
        D3DFMT_A8R8G8B8,
        D3DPOOL_MANAGED,
        D3DX_DEFAULT,
        D3DX_DEFAULT,
        keyColor,
        NULL,
        NULL,
        outTex);

    if (FAILED(hr) || !*outTex) {
        return false;
    }

    if (colorKey == HUD_ICON_PERSONA_BLUE) {
        ApplyPersonaBlueColorKey(*outTex);
    }
    else if (colorKey == HUD_ICON_JOKER_RED) {
        ApplyJokerColorKey(*outTex);
    }
    else if (colorKey == HUD_ICON_NARUKAMI_YELLOW) {
        ApplyNarukamiColorKey(*outTex);
    }
    else if (colorKey == HUD_ICON_YOSUKE_YELLOW) {
        ApplyYosukeColorKey(*outTex);
    }

    return true;
}

static void DrawHudIcon(LPD3DXSPRITE sprite, LPDIRECT3DTEXTURE9 tex, float x, float y, float size) {
    if (!sprite || !tex) return;

    D3DSURFACE_DESC desc;
    tex->GetLevelDesc(0, &desc);
    if (desc.Width <= 0 || desc.Height <= 0) return;

    float scale = size / (float)desc.Width;
    D3DXMATRIX matScale;
    D3DXMATRIX matTrans;
    D3DXMATRIX matFinal;
    D3DXMatrixScaling(&matScale, scale, scale, 1.0f);
    D3DXMatrixTranslation(&matTrans, x, y, 0.0f);
    matFinal = matScale * matTrans;
    sprite->SetTransform(&matFinal);

    D3DXVECTOR3 zeroPos(0.0f, 0.0f, 0.0f);
    sprite->Draw(tex, NULL, NULL, &zeroPos, D3DCOLOR_XRGB(255, 255, 255));

    D3DXMATRIX matIdentity;
    D3DXMatrixIdentity(&matIdentity);
    sprite->SetTransform(&matIdentity);
}

static float GetHudHealthChipDrainRate(int maxHealth) {
    if (HUD_HP_CHIP_DRAIN_FRAMES <= 0) return (float)maxHealth;
    return (float)maxHealth / (float)HUD_HP_CHIP_DRAIN_FRAMES;
}

// Tekken-style chip: damage peels the live bar immediately, white chip holds,
// then drains only after a quiet period with no new hits.
static void UpdateHudHealthTracker(HudHealthTracker& tracker, int health, int maxHealth) {
    if (maxHealth <= 0) return;

    if (tracker.syncedHealth < 0) {
        tracker.displayHealth = (float)health;
        tracker.syncedHealth = health;
        tracker.chipHoldFrames = 0;
        return;
    }

    if (health > tracker.syncedHealth) {
        // Heal / reset: snap both layers.
        tracker.displayHealth = (float)health;
        tracker.chipHoldFrames = 0;
    }
    else if (health < tracker.syncedHealth) {
        // New damage: keep chip at previous display (accumulate), restart hold.
        if (tracker.displayHealth < (float)tracker.syncedHealth) {
            tracker.displayHealth = (float)tracker.syncedHealth;
        }
        tracker.chipHoldFrames = HUD_HP_CHIP_HOLD_FRAMES;
    }

    tracker.syncedHealth = health;

    if (tracker.displayHealth <= (float)health) {
        tracker.displayHealth = (float)health;
        tracker.chipHoldFrames = 0;
        return;
    }

    if (tracker.chipHoldFrames > 0) {
        tracker.chipHoldFrames--;
        return;
    }

    tracker.displayHealth -= GetHudHealthChipDrainRate(maxHealth);
    if (tracker.displayHealth < (float)health) {
        tracker.displayHealth = (float)health;
    }
}

static void DrawMeterBar(
    LPD3DXSPRITE sprite,
    float x,
    float y,
    float width,
    float height,
    int value,
    int maxValue,
    bool anchorRight,
    D3DCOLOR fillColor)
{
    if (!sprite || maxValue <= 0) return;

    float ratio = (float)value / (float)maxValue;
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;

    DrawDebugRect(sprite, x - 2.0f, y - 2.0f, width + 4.0f, height + 4.0f, D3DCOLOR_ARGB(200, 255, 255, 255));
    DrawDebugRect(sprite, x, y, width, height, D3DCOLOR_ARGB(230, 16, 16, 20));

    float fillWidth = width * ratio;
    if (fillWidth <= 0.0f) return;

    float fillX = anchorRight ? (x + width - fillWidth) : x;
    DrawDebugRect(sprite, fillX, y, fillWidth, height, fillColor);
}

// 3 discrete stamina pips under SP (supports fractional fill on the active pip).
static void DrawStaminaSegments(
    LPD3DXSPRITE sprite,
    float x,
    float y,
    float width,
    float height,
    float stamina,
    float maxStamina,
    bool anchorRight,
    D3DCOLOR fillColor)
{
    if (!sprite || maxStamina <= 0.0f) return;

    const int segments = (int)(maxStamina + 0.5f);
    if (segments <= 0) return;

    const float gap = HUD_STAMINA_SEGMENT_GAP;
    const float segW = (width - gap * (segments - 1)) / (float)segments;
    float remaining = stamina;
    if (remaining < 0.0f) remaining = 0.0f;
    if (remaining > maxStamina) remaining = maxStamina;

    for (int i = 0; i < segments; ++i) {
        const int visualIndex = anchorRight ? (segments - 1 - i) : i;
        const float segX = x + (float)visualIndex * (segW + gap);
        DrawDebugRect(sprite, segX - 1.0f, y - 1.0f, segW + 2.0f, height + 2.0f, D3DCOLOR_ARGB(200, 255, 255, 255));
        DrawDebugRect(sprite, segX, y, segW, height, D3DCOLOR_ARGB(230, 16, 16, 20));

        float fill = remaining;
        if (fill > 1.0f) fill = 1.0f;
        if (fill < 0.0f) fill = 0.0f;
        if (fill > 0.0f) {
            const float fillW = segW * fill;
            const float fillX = anchorRight ? (segX + segW - fillW) : segX;
            DrawDebugRect(sprite, fillX, y, fillW, height, fillColor);
        }
        remaining -= 1.0f;
    }
}

static void DrawStreetFighterHealthBar(
    LPD3DXSPRITE sprite,
    float x,
    float y,
    float width,
    float height,
    int health,
    float displayHealth,
    int maxHealth,
    bool anchorRight)
{
    if (!sprite || maxHealth <= 0) return;

    float currentRatio = (float)health / (float)maxHealth;
    float delayedRatio = displayHealth / (float)maxHealth;
    if (currentRatio < 0.0f) currentRatio = 0.0f;
    if (currentRatio > 1.0f) currentRatio = 1.0f;
    if (delayedRatio < 0.0f) delayedRatio = 0.0f;
    if (delayedRatio > 1.0f) delayedRatio = 1.0f;
    if (delayedRatio < currentRatio) {
        delayedRatio = currentRatio;
    }

    const D3DCOLOR currentColor = D3DCOLOR_ARGB(255, HUD_HP_R, HUD_HP_G, HUD_HP_B);
    const D3DCOLOR delayedColor = D3DCOLOR_ARGB(255, HUD_HP_DELAYED_R, HUD_HP_DELAYED_G, HUD_HP_DELAYED_B);

    DrawDebugRect(sprite, x - 2.0f, y - 2.0f, width + 4.0f, height + 4.0f, D3DCOLOR_ARGB(200, 255, 255, 255));
    DrawDebugRect(sprite, x, y, width, height, D3DCOLOR_ARGB(230, 16, 16, 20));

    float currentWidth = width * currentRatio;
    float delayedWidth = width * delayedRatio;
    float chipWidth = delayedWidth - currentWidth;

    if (chipWidth > 0.5f) {
        float chipX = anchorRight ? (x + width - delayedWidth) : (x + currentWidth);
        DrawDebugRect(sprite, chipX, y, chipWidth, height, delayedColor);
    }

    if (currentWidth > 0.0f) {
        float currentX = anchorRight ? (x + width - currentWidth) : x;
        DrawDebugRect(sprite, currentX, y, currentWidth, height, currentColor);
    }
}

static LPDIRECT3DTEXTURE9 GetHudIconForCharacter(CharacterId id) {
    switch (id) {
    case Char_Joker: return g_JokerIconTex;
    case Char_Narukami: return g_NarukamiIconTex;
    case Char_Yosuke: return g_YosukeIconTex;
    case Char_Makoto:
    default: return g_MakotoIconTex;
    }
}

bool LoadHudTextures() {
    LoadHudFont();
    bool makotoOk = LoadHudIconTexture("assets/makoto/makoto_icon.png", &g_MakotoIconTex, HUD_ICON_PERSONA_BLUE);
    bool jokerOk = LoadHudIconTexture("assets/joker/joker_icon.png", &g_JokerIconTex, HUD_ICON_JOKER_RED);
    bool narukamiOk = LoadHudIconTexture("assets/narukami/narukami_icon.png", &g_NarukamiIconTex, HUD_ICON_NARUKAMI_YELLOW);
    bool yosukeOk = LoadHudIconTexture("assets/yosuke/yosuke_icon.png", &g_YosukeIconTex, HUD_ICON_YOSUKE_YELLOW);
    return makotoOk || jokerOk || narukamiOk || yosukeOk;
}

void CleanUpHudTextures() {
    if (g_HudFont) {
        g_HudFont->Release();
        g_HudFont = nullptr;
    }
    if (g_BannerFont) {
        g_BannerFont->Release();
        g_BannerFont = nullptr;
    }
    if (g_HudFontLoaded && g_LoadedHudFontPath) {
        RemoveFontResourceExA(g_LoadedHudFontPath, FR_PRIVATE, 0);
        g_LoadedHudFontPath = nullptr;
        g_HudFontLoaded = false;
    }
    if (g_MakotoIconTex) {
        g_MakotoIconTex->Release();
        g_MakotoIconTex = nullptr;
    }
    if (g_JokerIconTex) {
        g_JokerIconTex->Release();
        g_JokerIconTex = nullptr;
    }
    if (g_NarukamiIconTex) {
        g_NarukamiIconTex->Release();
        g_NarukamiIconTex = nullptr;
    }
    if (g_YosukeIconTex) {
        g_YosukeIconTex->Release();
        g_YosukeIconTex = nullptr;
    }
}

void NotifyHudDeviceLost() {
    if (g_HudFont) g_HudFont->OnLostDevice();
    if (g_BannerFont) g_BannerFont->OnLostDevice();
    if (g_TutorialFont) g_TutorialFont->OnLostDevice();
}

static void ResetHudFontDevice(ID3DXFont*& font, bool& loadedFlag) {
    if (!font) return;
    if (FAILED(font->OnResetDevice())) {
        font->Release();
        font = nullptr;
        loadedFlag = false;
    }
}

void NotifyHudDeviceReset() {
    ResetHudFontDevice(g_HudFont, g_HudFontLoaded);
    ResetHudFontDevice(g_BannerFont, g_HudFontLoaded);
    ResetHudFontDevice(g_TutorialFont, g_TutorialFontLoaded);
    LoadHudFont();
    LoadTutorialFont();
}

void ResetBattleHud(int p1MaxHealth, int p2MaxHealth) {
    g_P1HealthTrack.displayHealth = (float)p1MaxHealth;
    g_P1HealthTrack.syncedHealth = p1MaxHealth;
    g_P1HealthTrack.chipHoldFrames = 0;
    g_P2HealthTrack.displayHealth = (float)p2MaxHealth;
    g_P2HealthTrack.syncedHealth = p2MaxHealth;
    g_P2HealthTrack.chipHoldFrames = 0;
}

void SyncBattleHudHealth(int playerSlot, int health, int maxHealth) {
    (void)maxHealth;
    HudHealthTracker& tracker = (playerSlot == 1) ? g_P1HealthTrack : g_P2HealthTrack;
    if (tracker.syncedHealth < 0) {
        tracker.displayHealth = (float)health;
        tracker.syncedHealth = health;
        tracker.chipHoldFrames = 0;
        return;
    }
    // Heal / hard reset only. Damage chip is driven by UpdateHudHealthTracker in DrawBattleHud.
    if (health >= tracker.syncedHealth) {
        tracker.displayHealth = (float)health;
        tracker.syncedHealth = health;
        tracker.chipHoldFrames = 0;
    }
}

void DrawBattleHud(
    LPD3DXSPRITE sprite,
    int p1Health,
    int p1MaxHealth,
    int p1Sp,
    int p1MaxSp,
    float p1Stamina,
    float p1MaxStamina,
    int p2Health,
    int p2MaxHealth,
    int p2Sp,
    int p2MaxSp,
    float p2Stamina,
    float p2MaxStamina)
{
    if (!sprite) return;

    UpdateHudHealthTracker(g_P1HealthTrack, p1Health, p1MaxHealth);
    UpdateHudHealthTracker(g_P2HealthTrack, p2Health, p2MaxHealth);

    const D3DCOLOR spColor = D3DCOLOR_ARGB(255, HUD_SP_R, HUD_SP_G, HUD_SP_B);
    const D3DCOLOR staminaColor = D3DCOLOR_ARGB(255, HUD_STAMINA_R, HUD_STAMINA_G, HUD_STAMINA_B);
    const D3DCOLOR nameShadow = D3DCOLOR_ARGB(255, 0, 0, 0);
    const D3DCOLOR nameColor = D3DCOLOR_ARGB(255, 255, 255, 255);

    const float iconSize = HUD_ICON_SIZE;
    const float barWidth = HUD_BAR_WIDTH;
    const float barGap = HUD_BAR_GAP;
    const float iconBarGap = HUD_ICON_BAR_GAP;
    const float totalBarStackHeight =
        HUD_HP_BAR_HEIGHT + HUD_SP_BAR_HEIGHT + HUD_STAMINA_BAR_HEIGHT + barGap * 2.0f;

    const float p1IconX = HUD_EDGE_MARGIN;
    const float p1BarX = p1IconX + iconSize + iconBarGap;
    const float p2IconX = (float)SCREEN_WIDTH - HUD_EDGE_MARGIN - iconSize;
    const float p2BarX = p2IconX - iconBarGap - barWidth;

    const float iconY = HUD_TOP_Y;
    const float p1BarY = iconY + (iconSize - totalBarStackHeight) * 0.5f;
    const float p2BarY = p1BarY;

    const float p1NameX = p1BarX;
    const float p2NameEdgeX = p2BarX + barWidth;
    const float nameY = p1BarY + HUD_NAME_OFFSET_Y;

    const CharacterId p1Id = g_Player1 ? g_Player1->GetCharacterId() : Char_Makoto;
    const CharacterId p2Id = g_Player2 ? g_Player2->GetCharacterId() : Char_Joker;
    const char* p1Name = g_Player1 ? g_Player1->GetDisplayName() : GetCharacterDisplayName(p1Id);
    const char* p2Name = g_Player2 ? g_Player2->GetDisplayName() : GetCharacterDisplayName(p2Id);
    LPDIRECT3DTEXTURE9 p1Icon = GetHudIconForCharacter(p1Id);
    LPDIRECT3DTEXTURE9 p2Icon = GetHudIconForCharacter(p2Id);

    sprite->Begin(D3DXSPRITE_ALPHABLEND);
    DrawHudIcon(sprite, p1Icon, p1IconX, iconY, iconSize);
    DrawHudIcon(sprite, p2Icon, p2IconX, iconY, iconSize);
    sprite->End();

    const float p1SpY = p1BarY + HUD_HP_BAR_HEIGHT + barGap;
    const float p1StaminaY = p1SpY + HUD_SP_BAR_HEIGHT + barGap;
    const float p2SpY = p2BarY + HUD_HP_BAR_HEIGHT + barGap;
    const float p2StaminaY = p2SpY + HUD_SP_BAR_HEIGHT + barGap;

    DrawStreetFighterHealthBar(
        sprite, p1BarX, p1BarY, barWidth, HUD_HP_BAR_HEIGHT,
        p1Health, g_P1HealthTrack.displayHealth, p1MaxHealth, false);
    DrawMeterBar(sprite, p1BarX, p1SpY, barWidth, HUD_SP_BAR_HEIGHT,
        p1Sp, p1MaxSp, false, spColor);
    DrawStaminaSegments(sprite, p1BarX, p1StaminaY, barWidth, HUD_STAMINA_BAR_HEIGHT,
        p1Stamina, p1MaxStamina, false, staminaColor);

    DrawStreetFighterHealthBar(
        sprite, p2BarX, p2BarY, barWidth, HUD_HP_BAR_HEIGHT,
        p2Health, g_P2HealthTrack.displayHealth, p2MaxHealth, true);
    DrawMeterBar(sprite, p2BarX, p2SpY, barWidth, HUD_SP_BAR_HEIGHT,
        p2Sp, p2MaxSp, true, spColor);
    DrawStaminaSegments(sprite, p2BarX, p2StaminaY, barWidth, HUD_STAMINA_BAR_HEIGHT,
        p2Stamina, p2MaxStamina, true, staminaColor);

    DrawHudText(p1Name, p1NameX + 1.0f, nameY + 1.0f, nameShadow, false, 0.0f);
    DrawHudText(p1Name, p1NameX, nameY, nameColor, false, 0.0f);
    DrawHudText(p2Name, 0.0f, nameY + 1.0f, nameShadow, true, p2NameEdgeX + 1.0f);
    DrawHudText(p2Name, 0.0f, nameY, nameColor, true, p2NameEdgeX);
}

void DrawTutorialGuideOverlay() {
    if (!IsTutorialBattleMode() || IsBattleEndSequence()) return;
    if (IsTutorialGuideComplete()) return;
    if (!LoadTutorialFont() || !g_TutorialFont) return;

    const float panelX = 12.0f;
    const float panelY = (float)SCREEN_HEIGHT - 108.0f;
    const float panelW = 500.0f;
    const float panelH = 96.0f;

    DrawDebugRect(
        spriteBrush,
        panelX,
        panelY,
        panelW,
        panelH,
        D3DCOLOR_ARGB(190, 12, 12, 24));
    DrawDebugRect(
        spriteBrush,
        panelX,
        panelY,
        panelW,
        2.0f,
        D3DCOLOR_ARGB(255, 255, 210, 64));

    const D3DCOLOR titleColor = D3DCOLOR_XRGB(255, 220, 96);
    const D3DCOLOR bodyColor = D3DCOLOR_XRGB(220, 220, 220);
    const D3DCOLOR tagColor = D3DCOLOR_XRGB(120, 200, 255);
    const D3DCOLOR noteColor = D3DCOLOR_XRGB(180, 180, 180);

    RECT titleRect = {
        (LONG)(panelX + 10),
        (LONG)(panelY + 8),
        (LONG)(panelX + panelW - 110),
        (LONG)(panelY + 28)
    };
    g_TutorialFont->DrawTextA(
        nullptr,
        GetTutorialGuideObjective(),
        -1,
        &titleRect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS,
        titleColor);

    RECT bodyRect = {
        (LONG)(panelX + 10),
        (LONG)(panelY + 30),
        (LONG)(panelX + panelW - 10),
        (LONG)(panelY + 64)
    };
    g_TutorialFont->DrawTextA(
        nullptr,
        GetTutorialGuideDetail(),
        -1,
        &bodyRect,
        DT_LEFT | DT_WORDBREAK,
        bodyColor);

    RECT noteRect = {
        (LONG)(panelX + 10),
        (LONG)(panelY + 66),
        (LONG)(panelX + panelW - 10),
        (LONG)(panelY + 88)
    };
    g_TutorialFont->DrawTextA(
        nullptr,
        "Complete this move to unlock the next lesson.",
        -1,
        &noteRect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS,
        noteColor);

    RECT tagRect = {
        (LONG)(panelX + panelW - 108),
        (LONG)(panelY + 10),
        (LONG)(panelX + panelW - 8),
        (LONG)(panelY + 28)
    };
    char stepText[32];
    const int stepIndex = GetTutorialGuideStepIndex();
    const int stepCount = GetTutorialGuideStepCount();
    sprintf_s(stepText, "%d / %d", stepIndex + 1, stepCount);
    g_TutorialFont->DrawTextA(
        nullptr,
        stepText,
        -1,
        &tagRect,
        DT_RIGHT | DT_VCENTER | DT_SINGLELINE,
        tagColor);
}

void DrawBattleDebugHintOverlay() {
    if (!IsBattleCombatActive() || IsBattleEndSequence()) return;
    if (!LoadTutorialFont() || !g_TutorialFont) return;

    const bool tutorial = IsTutorialBattleMode();
    const char* hint = nullptr;
    if (tutorial) {
        hint = g_ShowDebugHitboxes
            ? "DEBUG: hitboxes ON (B off | H heal)"
            : "B: hitbox debug | H: heal HP";
    }
    else {
        hint = g_ShowDebugHitboxes
            ? "DEBUG: hitboxes ON (B to hide)"
            : "B: hitbox debug";
    }

    RECT rect = {
        10,
        (LONG)SCREEN_HEIGHT - 28,
        (LONG)SCREEN_WIDTH - 10,
        (LONG)SCREEN_HEIGHT - 8
    };
    g_TutorialFont->DrawTextA(
        nullptr,
        hint,
        -1,
        &rect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE,
        D3DCOLOR_XRGB(180, 220, 255));
}
