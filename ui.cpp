#include "ui.h"
#include "renderer.h"
#include <algorithm>

static LPDIRECT3DTEXTURE9 g_MakotoIconTex = nullptr;
static LPDIRECT3DTEXTURE9 g_JokerIconTex = nullptr;
static ID3DXFont* g_HudFont = nullptr;
static bool g_HudFontLoaded = false;

struct HudHealthTracker {
    float displayHealth = 0.0f;
    int syncedHealth = -1;
};

static HudHealthTracker g_P1HealthTrack;
static HudHealthTracker g_P2HealthTrack;

enum HudIconColorKey {
    HUD_ICON_NO_COLORKEY = 0,
    HUD_ICON_PERSONA_BLUE,
    HUD_ICON_JOKER_RED
};

static bool LoadHudFont() {
    if (g_HudFontLoaded) {
        return g_HudFont != nullptr;
    }

    if (AddFontResourceExA("assets/font/font.TTF", FR_PRIVATE, 0) == 0) {
        return false;
    }

    g_HudFontLoaded = true;
    HRESULT hr = D3DXCreateFontA(
        g_pD3DDevice,
        28,
        0,
        FW_BOLD,
        1,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        "BM space",
        &g_HudFont);

    return SUCCEEDED(hr) && g_HudFont != nullptr;
}

static void DrawHudText(const char* text, float x, float y, D3DCOLOR color, bool rightAlign, float alignEdgeX) {
    if (!g_HudFont || !text) return;

    RECT rect;
    if (rightAlign) {
        rect.left = (LONG)(alignEdgeX - 220.0f);
        rect.right = (LONG)alignEdgeX;
    }
    else {
        rect.left = (LONG)x;
        rect.right = (LONG)(x + 220.0f);
    }
    rect.top = (LONG)y;
    rect.bottom = (LONG)(y + 32.0f);

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

static float GetHudHealthDrainRate(int maxHealth) {
    return (float)maxHealth / 150.0f;
}

static void UpdateHudHealthTracker(HudHealthTracker& tracker, int health, int maxHealth) {
    if (maxHealth <= 0) return;

    if (tracker.syncedHealth < 0) {
        tracker.displayHealth = (float)health;
        tracker.syncedHealth = health;
        return;
    }

    if (health > tracker.syncedHealth) {
        tracker.displayHealth = (float)health;
    }
    else if (health < tracker.syncedHealth) {
        if (tracker.displayHealth < (float)tracker.syncedHealth) {
            tracker.displayHealth = (float)tracker.syncedHealth;
        }
    }

    tracker.syncedHealth = health;

    if (tracker.displayHealth > (float)health) {
        tracker.displayHealth -= GetHudHealthDrainRate(maxHealth);
        if (tracker.displayHealth < (float)health) {
            tracker.displayHealth = (float)health;
        }
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

bool LoadHudTextures() {
    LoadHudFont();
    bool makotoOk = LoadHudIconTexture("assets/makoto/makoto_icon.png", &g_MakotoIconTex, HUD_ICON_PERSONA_BLUE);
    bool jokerOk = LoadHudIconTexture("assets/joker/joker_icon.png", &g_JokerIconTex, HUD_ICON_JOKER_RED);
    return makotoOk || jokerOk;
}

void CleanUpHudTextures() {
    if (g_HudFont) {
        g_HudFont->Release();
        g_HudFont = nullptr;
    }
    if (g_HudFontLoaded) {
        RemoveFontResourceExA("assets/font/font.TTF", FR_PRIVATE, 0);
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
}

void ResetBattleHud(int p1MaxHealth, int p2MaxHealth) {
    g_P1HealthTrack.displayHealth = (float)p1MaxHealth;
    g_P1HealthTrack.syncedHealth = p1MaxHealth;
    g_P2HealthTrack.displayHealth = (float)p2MaxHealth;
    g_P2HealthTrack.syncedHealth = p2MaxHealth;
}

void SyncBattleHudHealth(int playerSlot, int health, int maxHealth) {
    HudHealthTracker& tracker = (playerSlot == 1) ? g_P1HealthTrack : g_P2HealthTrack;
    tracker.displayHealth = (float)health;
    tracker.syncedHealth = health;
}

void DrawBattleHud(
    LPD3DXSPRITE sprite,
    int p1Health,
    int p1MaxHealth,
    int p1Sp,
    int p1MaxSp,
    int p2Health,
    int p2MaxHealth,
    int p2Sp,
    int p2MaxSp)
{
    if (!sprite) return;

    UpdateHudHealthTracker(g_P1HealthTrack, p1Health, p1MaxHealth);
    UpdateHudHealthTracker(g_P2HealthTrack, p2Health, p2MaxHealth);

    const D3DCOLOR spColor = D3DCOLOR_ARGB(255, HUD_SP_R, HUD_SP_G, HUD_SP_B);
    const D3DCOLOR nameShadow = D3DCOLOR_ARGB(255, 0, 0, 0);
    const D3DCOLOR nameColor = D3DCOLOR_ARGB(255, 255, 255, 255);

    const float iconSize = HUD_ICON_SIZE;
    const float barWidth = HUD_BAR_WIDTH;
    const float barGap = HUD_BAR_GAP;
    const float iconBarGap = HUD_ICON_BAR_GAP;
    const float totalBarStackHeight = HUD_HP_BAR_HEIGHT + HUD_SP_BAR_HEIGHT + barGap;

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

    sprite->Begin(D3DXSPRITE_ALPHABLEND);
    DrawHudIcon(sprite, g_MakotoIconTex, p1IconX, iconY, iconSize);
    DrawHudIcon(sprite, g_JokerIconTex, p2IconX, iconY, iconSize);
    sprite->End();

    DrawStreetFighterHealthBar(
        sprite, p1BarX, p1BarY, barWidth, HUD_HP_BAR_HEIGHT,
        p1Health, g_P1HealthTrack.displayHealth, p1MaxHealth, false);
    DrawMeterBar(sprite, p1BarX, p1BarY + HUD_HP_BAR_HEIGHT + barGap, barWidth, HUD_SP_BAR_HEIGHT,
        p1Sp, p1MaxSp, false, spColor);

    DrawStreetFighterHealthBar(
        sprite, p2BarX, p2BarY, barWidth, HUD_HP_BAR_HEIGHT,
        p2Health, g_P2HealthTrack.displayHealth, p2MaxHealth, true);
    DrawMeterBar(sprite, p2BarX, p2BarY + HUD_HP_BAR_HEIGHT + barGap, barWidth, HUD_SP_BAR_HEIGHT,
        p2Sp, p2MaxSp, true, spColor);

    DrawHudText("MAKOTO", p1NameX + 1.0f, nameY + 1.0f, nameShadow, false, 0.0f);
    DrawHudText("MAKOTO", p1NameX, nameY, nameColor, false, 0.0f);
    DrawHudText("JOKER", 0.0f, nameY + 1.0f, nameShadow, true, p2NameEdgeX + 1.0f);
    DrawHudText("JOKER", 0.0f, nameY, nameColor, true, p2NameEdgeX);
}
