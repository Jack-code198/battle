#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <dinput.h>
#include <cmath>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

#include "collision.h"

// window / screen
extern HWND g_hWnd;
extern WNDCLASS wndClass;
extern MSG msg;
extern bool g_WindowHasFocus;
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;

// 角色渲染 / 物理
inline constexpr float CHARACTER_SCREEN_HEIGHT_RATIO = 0.22f;
inline constexpr float MAKOTO_SCREEN_HEIGHT_RATIO = 0.24f;
inline constexpr float CHARACTER_REFERENCE_HEIGHT = 64.0f;
inline constexpr float CHARACTER_REFERENCE_WIDTH = 48.0f;
inline constexpr float MAKOTO_BODY_HEIGHT = 56.0f;
inline constexpr float MAKOTO_FEET_Y = 56.0f;
inline constexpr float MAKOTO_BODY_WIDTH = 22.0f;
inline constexpr float MAKOTO_BODY_CENTER_X = 11.0f;
inline constexpr float MAKOTO_WINDOW_MARGIN = 12.0f;
inline constexpr float MAKOTO_SPAWN_FORWARD = 200.0f;
inline constexpr float JOKER_SPAWN_X = 680.0f;
inline constexpr float OPPONENT_SPAWN_X = JOKER_SPAWN_X;
inline constexpr float OPPONENT_SKILL_ANCHOR_RATIO = 0.45f;

inline float GetBattleGroundY() {
    return (float)SCREEN_HEIGHT * (980.0f / 1080.0f);
}
#define CHARACTER_GROUND_Y GetBattleGroundY()
inline constexpr float GRAVITY = 0.55f;
inline constexpr float MESSIAH_REFERENCE_HEIGHT = 152.0f;
inline constexpr float MEGIDOLAON_REFERENCE_HEIGHT = 176.0f;
inline constexpr float THANATOS_MAZIODYNE_FORWARD_OFFSET = 50.0f;
inline constexpr float THANATOS_MAZIODYNE_VERTICAL_OFFSET = 350.0f;
inline constexpr float MAZIODYNE_HORIZONTAL_OFFSET = 50.0f;
inline constexpr float MAZIODYNE_VERTICAL_OFFSET = 80.0f;
inline constexpr float PERSONA_BEHIND_HORIZONTAL = 90.0f;
inline constexpr float ORPHEUS_BEHIND_HORIZONTAL = 145.0f;
inline constexpr float JACKFROST_BEHIND_VERTICAL = 110.0f;
inline constexpr float ORPHEUS_BEHIND_VERTICAL = 130.0f;
inline constexpr float MESSIAH_HORIZONTAL_OFFSET = 350.0f;
inline constexpr float MESSIAH_VERTICAL_OFFSET = 300.0f;
inline constexpr float MEGIDOLAON_HORIZONTAL_OFFSET = 700.0f;
inline constexpr float MEGIDOLAON_VERTICAL_OFFSET = 500.0f;
inline constexpr float THANATOS_SLASH_HORIZONTAL_OFFSET = 0.0f;
inline constexpr float THANATOS_SLASH_VERTICAL_OFFSET = -70.0f;
inline constexpr float DODGE_SLIDE_SPEED = 2.5f;
inline constexpr float OPPONENT_RETURN_SPEED = 8.0f;
inline constexpr int OPPONENT_STAND_ANIM_TICKS = 8;
inline constexpr int OPPONENT_WALK_ANIM_TICKS = 6;
inline constexpr int OPPONENT_DAMAGE_ANIM_TICKS = 5;

inline constexpr float JOKER_BODY_WIDTH = 22.0f;
inline constexpr float JOKER_HURTBOX_WIDTH = 26.0f;
inline constexpr float JOKER_HURTBOX_HEIGHT = 56.0f;
inline constexpr float JOKER_PUSHBOX_WIDTH = 20.0f;
inline constexpr float JOKER_PUSHBOX_HEIGHT = 52.0f;

inline constexpr bool JOKER_SANDBAG_MODE = true;
inline constexpr bool OPPONENT_SANDBAG_MODE = JOKER_SANDBAG_MODE;
inline constexpr bool TRAINING_MODE = true;
inline constexpr int TRAINING_HEAL_IDLE_FRAMES = 45;

inline constexpr float OPPONENT_MELEE_KNOCKBACK = 6.0f;
inline constexpr float OPPONENT_SKILL_KNOCKBACK = 0.0f;

inline constexpr float JOKER_BODY_HEIGHT = 56.0f;
inline constexpr float JOKER_FEET_Y = 56.0f;
inline constexpr float ARSENE_BODY_HEIGHT = 56.0f;
inline constexpr float ARSENE_FEET_Y = 56.0f;
inline constexpr float ARSENE_BEHIND_HORIZONTAL = 95.0f;
inline constexpr float ARSENE_BEHIND_VERTICAL = 125.0f;
inline constexpr float AGI_EFFECT_SCALE = 1.0f;
inline constexpr float MABUFU_EFFECT_SCALE = 1.0f;
inline constexpr float THANATOS_SLASH_EFFECT_SCALE = 1.0f;
inline constexpr float PERSONA_EFFECT_SCALE = 4.5f;
inline constexpr float AGI_MABUFU_OFFSET_X = 14.0f;
inline constexpr float AGI_MABUFU_OFFSET_Y = 8.0f;
inline constexpr int ACTION_VISUAL_HOLD_FRAMES = 0;
inline constexpr int INTRO_TO_STANCE_BLEND_FRAMES = 0;
inline constexpr int INTRO_VISUAL_HOLD_FRAMES = 0;
inline constexpr int PERSONA_ANIM_DELAY = 5;
inline constexpr int PERSONA_EFFECT_ANIM_DELAY = 3;
inline constexpr int THANATOS_SLASH_ANIM_DELAY = 4;
inline constexpr int PERSONA_STANCE_ANIM_TICKS = 6;
inline constexpr int MAKOTO_INTRO_TICKS = 14;
inline constexpr int MAKOTO_SUMMON_AIR_TICKS = 6;
inline constexpr int IDLE_THRESHOLD_FRAMES = 600;
inline constexpr int MEGIDOLAON_BURST_FRAME_COUNT = 8;
inline constexpr int MESSIAH_SUMMON_FRAME_COUNT = 18;
inline constexpr int MESSIAH_EFFECT_FRAME_COUNT = 8;
inline constexpr int MEGIDOLAON_EFFECT_FRAME_COUNT = 16;
inline constexpr float MEGIDOLAON_BURST_SCALE_FACTOR = 3.0f;
inline constexpr float EFFECT_ANCHOR_CENTER = 0.5f;
inline constexpr float EFFECT_ANCHOR_BOTTOM = 1.0f;
inline constexpr BYTE HUD_HP_R = 156;
inline constexpr BYTE HUD_HP_G = 247;
inline constexpr BYTE HUD_HP_B = 240;
inline constexpr BYTE HUD_SP_R = 253;
inline constexpr BYTE HUD_SP_G = 251;
inline constexpr BYTE HUD_SP_B = 140;
inline constexpr BYTE HUD_HP_DELAYED_R = 255;
inline constexpr BYTE HUD_HP_DELAYED_G = 255;
inline constexpr BYTE HUD_HP_DELAYED_B = 255;

inline constexpr int SP_COST_SUMMON_1 = 20;
inline constexpr int SP_COST_SUMMON_2 = 20;
inline constexpr int SP_COST_SUMMON_AIR = 35;
inline constexpr int SP_COST_SUMMON_AIR_2 = 45;
inline constexpr int SP_COST_THANATOS_SLASH = 25;
inline constexpr int SP_GAIN_ON_HIT = 15;

inline constexpr int MAKOTO_MAX_HEALTH = 400;
inline constexpr int JOKER_MAX_HEALTH = 400;
inline constexpr int FIGHTER_MAX_SP = 100;

inline constexpr float HUD_EDGE_MARGIN = 40.0f;
inline constexpr float HUD_TOP_Y = 18.0f;
inline constexpr float HUD_ICON_SIZE = 56.0f;
inline constexpr float HUD_ICON_BAR_GAP = 12.0f;
inline constexpr float HUD_BAR_WIDTH = 240.0f;
inline constexpr float HUD_HP_BAR_HEIGHT = 16.0f;
inline constexpr float HUD_SP_BAR_HEIGHT = 10.0f;
inline constexpr float HUD_BAR_GAP = 5.0f;
inline constexpr float HUD_NAME_OFFSET_Y = -22.0f;
inline constexpr int MAKOTO_CELL_SIZE = 256;
inline constexpr int GAME_ANIMATION_FPS = 60;
inline constexpr int MAKOTO_LOOP_TICKS_SLOW = 4;
inline constexpr int MAKOTO_LOOP_TICKS_FAST = 3;
inline constexpr int MAKOTO_IDLE_PLAY_TICKS = 6;
inline constexpr int MAKOTO_ACTION_TICKS = 3;
inline constexpr int MAKOTO_SUMMON_TICKS = 4;
inline constexpr int MAKOTO_IDLE_TICKS = 15;

struct SpriteSheetBounds {
    float maxFrameWidth;
    float maxFrameHeight;
};

inline SpriteSheetBounds MeasureTextureRange(LPDIRECT3DTEXTURE9* textures, int startIndex, int endIndex) {
    SpriteSheetBounds bounds = { 0.0f, 0.0f };
    for (int textureIndex = startIndex; textureIndex < endIndex; ++textureIndex) {
        if (textures[textureIndex] == NULL) {
            continue;
        }
        D3DSURFACE_DESC surfaceDescription;
        textures[textureIndex]->GetLevelDesc(0, &surfaceDescription);
        if ((float)surfaceDescription.Width > bounds.maxFrameWidth) {
            bounds.maxFrameWidth = (float)surfaceDescription.Width;
        }
        if ((float)surfaceDescription.Height > bounds.maxFrameHeight) {
            bounds.maxFrameHeight = (float)surfaceDescription.Height;
        }
    }
    return bounds;
}

extern SpriteSheetBounds g_MessiahSheetBounds;
extern SpriteSheetBounds g_MegidolaonBurstBounds;
extern SpriteSheetBounds g_MegidolaonBlastBounds;

inline float GetCharacterRenderScale() {
    return ((float)SCREEN_HEIGHT * CHARACTER_SCREEN_HEIGHT_RATIO) / CHARACTER_REFERENCE_HEIGHT;
}

inline float GetMakotoDrawScale() {
    return ((float)SCREEN_HEIGHT * MAKOTO_SCREEN_HEIGHT_RATIO) / MAKOTO_BODY_HEIGHT;
}

inline float GetJokerDrawScale() {
    return ((float)SCREEN_HEIGHT * MAKOTO_SCREEN_HEIGHT_RATIO) / JOKER_BODY_HEIGHT;
}

inline float GetPersonaEffectDrawScale() {
    return ((float)SCREEN_HEIGHT * MAKOTO_SCREEN_HEIGHT_RATIO) / (float)MAKOTO_CELL_SIZE;
}

inline float GetMakotoScreenHalfWidth() {
    return MAKOTO_BODY_WIDTH * 0.5f * GetMakotoDrawScale();
}

inline float GetJokerScreenHalfWidth() {
    return JOKER_PUSHBOX_WIDTH * 0.5f * GetJokerDrawScale();
}

inline void ClampFighterCenterX(float& centerX, float bodyHalfWidth) {
    float minX = bodyHalfWidth + MAKOTO_WINDOW_MARGIN;
    float maxX = (float)SCREEN_WIDTH - minX;
    if (centerX < minX) centerX = minX;
    if (centerX > maxX) centerX = maxX;
}

inline void ClampMakotoCenterX(float& centerX) {
    ClampFighterCenterX(centerX, GetMakotoScreenHalfWidth());
}

inline void ClampJokerCenterX(float& centerX) {
    ClampFighterCenterX(centerX, GetJokerScreenHalfWidth());
}

inline float GetEffectRenderScale() {
    return ((float)SCREEN_HEIGHT * 0.18f) / CHARACTER_REFERENCE_HEIGHT;
}

inline float GetMessiahRenderScale() {
    return ((float)SCREEN_HEIGHT * 0.26f) / MESSIAH_REFERENCE_HEIGHT;
}

inline float GetMegidolaonRenderScale() {
    return ((float)SCREEN_HEIGHT * 0.16f) / MEGIDOLAON_REFERENCE_HEIGHT;
}

inline constexpr BYTE PERSONA_COLORKEY_R = 7;
inline constexpr BYTE PERSONA_COLORKEY_G = 115;
inline constexpr BYTE PERSONA_COLORKEY_B = 255;

inline constexpr BYTE JOKER_COLORKEY_R = 232;
inline constexpr BYTE JOKER_COLORKEY_G = 4;
inline constexpr BYTE JOKER_COLORKEY_B = 4;

inline void ApplyTextureColorKey(LPDIRECT3DTEXTURE9 tex, BYTE keyR, BYTE keyG, BYTE keyB) {
    if (!tex) return;

    D3DSURFACE_DESC desc;
    tex->GetLevelDesc(0, &desc);

    D3DLOCKED_RECT rect;
    if (FAILED(tex->LockRect(0, &rect, NULL, 0))) return;

    for (UINT y = 0; y < desc.Height; y++) {
        DWORD* row = (DWORD*)((BYTE*)rect.pBits + y * rect.Pitch);
        for (UINT x = 0; x < desc.Width; x++) {
            DWORD pixel = row[x];
            BYTE r = (pixel >> 16) & 0xFF;
            BYTE g = (pixel >> 8) & 0xFF;
            BYTE b = pixel & 0xFF;
            if (r == keyR && g == keyG && b == keyB) {
                row[x] = 0x00000000;
            }
        }
    }

    tex->UnlockRect(0);
}

inline void ApplyJokerColorKey(LPDIRECT3DTEXTURE9 tex) {
    ApplyTextureColorKey(tex, JOKER_COLORKEY_R, JOKER_COLORKEY_G, JOKER_COLORKEY_B);
}

inline void ApplyPersonaBlueColorKey(LPDIRECT3DTEXTURE9 tex) {
    ApplyTextureColorKey(tex, PERSONA_COLORKEY_R, PERSONA_COLORKEY_G, PERSONA_COLORKEY_B);
}

inline void DrawScaledCharacterSprite(
    LPD3DXSPRITE sprite,
    LPDIRECT3DTEXTURE9 tex,
    const RECT* srcRect,
    const D3DXVECTOR3& pos,
    int facingDirection,
    float scale,
    D3DCOLOR color,
    float sourceContentHeight = 0.0f,
    float sourceFeetY = 0.0f)
{
    if (!sprite || !tex) return;

    int srcW = MAKOTO_CELL_SIZE;
    int srcH = MAKOTO_CELL_SIZE;
    if (srcRect) {
        srcW = srcRect->right - srcRect->left;
        srcH = srcRect->bottom - srcRect->top;
    }
    else {
        D3DSURFACE_DESC desc;
        tex->GetLevelDesc(0, &desc);
        srcW = (int)desc.Width;
        srcH = (int)desc.Height;
    }

    float screenHeightRatio = (sourceContentHeight > 0.0f)
        ? MAKOTO_SCREEN_HEIGHT_RATIO
        : CHARACTER_SCREEN_HEIGHT_RATIO;
    float targetScreenHeight = (float)SCREEN_HEIGHT * screenHeightRatio;
    float contentHeight = (sourceContentHeight > 0.0f) ? sourceContentHeight : (float)srcH;
    float drawScale = targetScreenHeight / contentHeight;
    if (sourceContentHeight > 0.0f && scale > 0.0f) {
        drawScale *= scale;
    }
    float drawW = (float)srcW * drawScale;
    float drawH = (float)srcH * drawScale;
    float feetY = (sourceFeetY > 0.0f) ? sourceFeetY : (float)srcH;
    float anchorX = (sourceContentHeight > 0.0f) ? MAKOTO_BODY_CENTER_X : (drawW * 0.5f);
    float offsetY = pos.y - feetY * drawScale;
    float offsetX = (facingDirection == -1) ? pos.x + anchorX * drawScale : pos.x - anchorX * drawScale;

    D3DXMATRIX matFlip, matScale, matTrans, matFinal;
    if (facingDirection == -1) {
        D3DXMatrixScaling(&matFlip, -1.0f, 1.0f, 1.0f);
    }
    else {
        D3DXMatrixIdentity(&matFlip);
    }
    D3DXMatrixScaling(&matScale, drawScale, drawScale, 1.0f);
    D3DXMatrixTranslation(&matTrans, offsetX, offsetY, pos.z);
    matFinal = matFlip * matScale * matTrans;
    sprite->SetTransform(&matFinal);

    D3DXVECTOR3 zeroPos(0.0f, 0.0f, 0.0f);
    sprite->Draw(tex, srcRect, NULL, &zeroPos, color);

    D3DXMATRIX matIdentity;
    D3DXMatrixIdentity(&matIdentity);
    sprite->SetTransform(&matIdentity);
}

inline void DrawCenteredEffectSprite(
    LPD3DXSPRITE sprite,
    LPDIRECT3DTEXTURE9 tex,
    const RECT* srcRect,
    const D3DXVECTOR3& centerPos,
    float scale,
    D3DCOLOR color,
    float sourceContentHeight = 0.0f)
{
    if (!sprite || !tex) return;

    int srcW = 0;
    int srcH = 0;
    if (srcRect) {
        srcW = srcRect->right - srcRect->left;
        srcH = srcRect->bottom - srcRect->top;
    }
    else {
        D3DSURFACE_DESC desc;
        tex->GetLevelDesc(0, &desc);
        srcW = (int)desc.Width;
        srcH = (int)desc.Height;
    }

    float screenHeightRatio = (sourceContentHeight > 0.0f)
        ? MAKOTO_SCREEN_HEIGHT_RATIO
        : CHARACTER_SCREEN_HEIGHT_RATIO;
    float targetScreenHeight = (float)SCREEN_HEIGHT * screenHeightRatio;
    float contentHeight = (sourceContentHeight > 0.0f) ? sourceContentHeight : (float)srcH;
    float drawScale = targetScreenHeight / contentHeight;
    if (scale > 0.0f) {
        drawScale *= scale;
    }
    float drawW = (float)srcW * drawScale;
    float drawH = (float)srcH * drawScale;
    float offsetX = centerPos.x - drawW * 0.5f;
    float offsetY = centerPos.y - drawH * 0.5f;

    D3DXMATRIX matScale, matTrans, matFinal;
    D3DXMatrixScaling(&matScale, drawScale, drawScale, 1.0f);
    D3DXMatrixTranslation(&matTrans, offsetX, offsetY, centerPos.z);
    matFinal = matScale * matTrans;
    sprite->SetTransform(&matFinal);

    D3DXVECTOR3 zeroPos(0.0f, 0.0f, 0.0f);
    sprite->Draw(tex, srcRect, NULL, &zeroPos, color);

    D3DXMATRIX matIdentity;
    D3DXMatrixIdentity(&matIdentity);
    sprite->SetTransform(&matIdentity);
}

inline void DrawAnchoredEffectSprite(
    LPD3DXSPRITE sprite,
    LPDIRECT3DTEXTURE9 tex,
    const D3DXVECTOR3& anchorPos,
    float scale,
    D3DCOLOR color,
    float refWidth,
    float refHeight,
    float anchorNormX,
    float anchorNormY)
{
    if (!sprite || !tex) return;

    D3DSURFACE_DESC desc;
    tex->GetLevelDesc(0, &desc);
    float drawW = (float)desc.Width * scale;
    float drawH = (float)desc.Height * scale;
    float refDrawW = refWidth * scale;
    float refDrawH = refHeight * scale;
    float offsetX = anchorPos.x - refDrawW * anchorNormX + (refDrawW - drawW) * 0.5f;
    float offsetY = anchorPos.y - refDrawH * anchorNormY + (refDrawH - drawH) * 0.5f;

    D3DXMATRIX matScale, matTrans, matFinal;
    D3DXMatrixScaling(&matScale, scale, scale, 1.0f);
    D3DXMatrixTranslation(&matTrans, offsetX, offsetY, anchorPos.z);
    matFinal = matScale * matTrans;
    sprite->SetTransform(&matFinal);

    D3DXVECTOR3 zeroPos(0.0f, 0.0f, 0.0f);
    sprite->Draw(tex, NULL, NULL, &zeroPos, color);

    D3DXMATRIX matIdentity;
    D3DXMatrixIdentity(&matIdentity);
    sprite->SetTransform(&matIdentity);
}

// ============ Direct3D ============
extern IDirect3D9* g_pD3D;
extern IDirect3DDevice9* g_pD3DDevice;
extern D3DPRESENT_PARAMETERS d3dpp;
extern LPD3DXSPRITE spriteBrush;

// ============ DirectInput ============
extern LPDIRECTINPUT8 dInput;
extern LPDIRECTINPUTDEVICE8 dInputKeyboardDevice;
extern BYTE diKeys[256];

struct AttackData {
    int startFrame;
    int endFrame;
    int damage;
    float offsetX;
    float offsetY;
    float width;
    float height;
};

void DrawDebugRect(LPD3DXSPRITE sprite, float x, float y, float w, float h, D3DCOLOR color);

// ============ 外部攻击数据结构 ============
extern AttackData attackHitbox;
extern AttackData sideAttackHitbox;
extern AttackData attackUpHitbox;
extern AttackData downAttackHitbox;

// ============ 背景纹理声明 ============
extern LPDIRECT3DTEXTURE9 texBgCity1;