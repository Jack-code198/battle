#include "playerSelect.h"
#include "game_logic.h"
#include "input.h"
#include "audio.h"
#include "player/CharacterId.h"
#include <cstdio>

// ---------------------------------------------------------------------------
// Roster + layout (named constants — no magic RECT literals in draw/input)
// ---------------------------------------------------------------------------

struct PlayerSelectSlot {
    CharacterId id;
    LPDIRECT3DTEXTURE9 texture;
};

static PlayerSelectSlot g_Roster[] = {
    { Char_Makoto,   NULL },
    { Char_Joker,    NULL },
    { Char_Narukami, NULL },
    { Char_Yosuke,   NULL },
};
static const int ROSTER_COUNT = sizeof(g_Roster) / sizeof(g_Roster[0]);

static ID3DXFont* g_PlayerSelectFont = nullptr;
static ID3DXFont* g_PlayerSelectTitleFont = nullptr;
static ID3DXFont* g_PlayerSelectHintFont = nullptr;
static const char* g_LoadedPlayerSelectFontPath = nullptr;

static bool g_PlayerSelectLeftHeld = false;
static bool g_PlayerSelectRightHeld = false;
static bool g_PlayerSelectEnterHeld = false;
static bool g_PlayerSelectBackHeld = false;
static bool g_PlayerSelectClickHeld = false;

// Phase 0 = choosing P1, phase 1 = choosing P2, phase 2 = battle / tutorial mode.
static int g_SelectPhase = 0;
static int g_HighlightIndex = 0;
static int g_ModeHighlight = 0;
static CharacterId g_PendingP1 = Char_Makoto;
static int g_LastHoveredPortrait = -1;
static int g_LastHoveredModeOption = -1;

static RECT g_PortraitHits[ROSTER_COUNT] = {};
static RECT g_ModeOptionHits[2] = {};

static const D3DCOLOR PLAYER_SELECT_COLOR_NORMAL = D3DCOLOR_XRGB(255, 255, 255);
static const D3DCOLOR PLAYER_SELECT_COLOR_SELECTED = D3DCOLOR_XRGB(255, 230, 80);
static const D3DCOLOR PLAYER_SELECT_COLOR_DIM = D3DCOLOR_XRGB(170, 170, 170);
static const D3DCOLOR PLAYER_SELECT_FRAME_COLOR = D3DCOLOR_ARGB(220, 255, 230, 80);
static const D3DCOLOR PLAYER_SELECT_FRAME_IDLE = D3DCOLOR_ARGB(160, 255, 255, 255);

static const float PORTRAIT_SIZE = 180.0f;
static const float PORTRAIT_GAP = 48.0f;
static const float PORTRAIT_TOP = 200.0f;
static const float PORTRAIT_FRAME_THICKNESS = 4.0f;
// Gap between portrait bottom edge and the name label under it.
static const float PORTRAIT_NAME_GAP = 28.0f;

static const LONG PLAYER_SELECT_TITLE_TOP = 40;
static const LONG PLAYER_SELECT_TITLE_BOTTOM = 100;
static const LONG PLAYER_SELECT_VS_TOP = 110;
static const LONG PLAYER_SELECT_VS_BOTTOM = 160;
static const LONG PLAYER_SELECT_HINT_TOP = 700;
static const LONG PLAYER_SELECT_HINT_BOTTOM = 740;
static const LONG PLAYER_SELECT_HINT_SIDE_MARGIN = 40;
static const LONG PLAYER_SELECT_LABEL_HEIGHT = 28;

static const float MODE_OPTION_WIDTH = 320.0f;
static const float MODE_OPTION_HEIGHT = 72.0f;
static const float MODE_OPTION_GAP = 64.0f;
static const float MODE_OPTION_TOP = 280.0f;

static const char* GetBattleModeLabel(BattleMode mode) {
    return (mode == BattleMode::Tutorial) ? "TUTORIAL MODE (SAND BAG)" : "BATTLE MODE";
}

static void GetModeOptionLayout(int index, float& x, float& y, float& w, float& h) {
    const float totalWidth = MODE_OPTION_WIDTH * 2.0f + MODE_OPTION_GAP;
    const float startX = ((float)SCREEN_WIDTH - totalWidth) * 0.5f;
    x = startX + (float)index * (MODE_OPTION_WIDTH + MODE_OPTION_GAP);
    y = MODE_OPTION_TOP;
    w = MODE_OPTION_WIDTH;
    h = MODE_OPTION_HEIGHT;
}

static void UpdateModeOptionHitboxes() {
    for (int i = 0; i < 2; i++) {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
        GetModeOptionLayout(i, x, y, w, h);
        g_ModeOptionHits[i] = {
            (LONG)x,
            (LONG)y,
            (LONG)(x + w),
            (LONG)(y + h)
        };
    }
}

// ---------------------------------------------------------------------------
// Fonts
// ---------------------------------------------------------------------------

static bool CreatePlayerSelectUiFont(const char* familyName, INT height, ID3DXFont** outFont) {
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

static bool LoadOrCreatePlayerSelectUiFont(INT height, ID3DXFont** outFont) {
    if (*outFont) return true;

    if (!CreatePlayerSelectUiFont(NORMAL_FONT_FAMILY, height, outFont)) {
        if (*outFont) {
            (*outFont)->Release();
            *outFont = nullptr;
        }
        if (!CreatePlayerSelectUiFont("Arial", height, outFont)) {
            *outFont = nullptr;
            return false;
        }
    }
    return true;
}

static bool LoadPlayerSelectFont() {
    if (g_PlayerSelectFont && g_PlayerSelectTitleFont && g_PlayerSelectHintFont) {
        return true;
    }

    if (!g_LoadedPlayerSelectFontPath) {
        if (AddFontResourceExA(NORMAL_FONT_FILE, FR_PRIVATE, 0) != 0) {
            g_LoadedPlayerSelectFontPath = NORMAL_FONT_FILE;
        }
    }

    bool ok = true;
    if (!LoadOrCreatePlayerSelectUiFont(36, &g_PlayerSelectTitleFont)) ok = false;
    if (!LoadOrCreatePlayerSelectUiFont(16, &g_PlayerSelectFont)) ok = false;
    if (!LoadOrCreatePlayerSelectUiFont(16, &g_PlayerSelectHintFont)) ok = false;
    return ok;
}

// ---------------------------------------------------------------------------
// Texture load / cleanup / device notifications
// ---------------------------------------------------------------------------

bool LoadPlayerSelectTextures() {
    bool allOk = true;

    for (int i = 0; i < ROSTER_COUNT; i++) {
        if (g_Roster[i].texture != NULL) continue;

        D3DCOLOR keyColor = 0;
        if (g_Roster[i].id == Char_Makoto) {
            keyColor = D3DCOLOR_XRGB(PERSONA_COLORKEY_R, PERSONA_COLORKEY_G, PERSONA_COLORKEY_B);
        }
        else if (g_Roster[i].id == Char_Joker) {
            keyColor = D3DCOLOR_XRGB(JOKER_COLORKEY_R, JOKER_COLORKEY_G, JOKER_COLORKEY_B);
        }
        else if (g_Roster[i].id == Char_Narukami) {
            keyColor = D3DCOLOR_XRGB(NARUKAMI_COLORKEY_R, NARUKAMI_COLORKEY_G, NARUKAMI_COLORKEY_B);
        }
        else if (g_Roster[i].id == Char_Yosuke) {
            keyColor = D3DCOLOR_XRGB(YOSUKE_COLORKEY_R, YOSUKE_COLORKEY_G, YOSUKE_COLORKEY_B);
        }

        HRESULT hr = D3DXCreateTextureFromFileEx(
            g_pD3DDevice,
            GetCharacterIconPath(g_Roster[i].id),
            D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT,
            NULL, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT,
            keyColor, NULL, NULL, &g_Roster[i].texture);

        if (FAILED(hr)) {
            g_Roster[i].texture = NULL;
            allOk = false;
            continue;
        }

        if (g_Roster[i].id == Char_Makoto) {
            ApplyPersonaBlueColorKey(g_Roster[i].texture);
        }
        else if (g_Roster[i].id == Char_Joker) {
            ApplyJokerColorKey(g_Roster[i].texture);
        }
        else if (g_Roster[i].id == Char_Narukami) {
            ApplyNarukamiColorKey(g_Roster[i].texture);
        }
        else if (g_Roster[i].id == Char_Yosuke) {
            ApplyYosukeColorKey(g_Roster[i].texture);
        }
    }

    if (!LoadPlayerSelectFont()) {
        allOk = false;
    }

    return allOk;
}

void CleanUpPlayerSelectTextures() {
    for (int i = 0; i < ROSTER_COUNT; i++) {
        if (g_Roster[i].texture) {
            g_Roster[i].texture->Release();
            g_Roster[i].texture = NULL;
        }
    }

    if (g_PlayerSelectFont) {
        g_PlayerSelectFont->Release();
        g_PlayerSelectFont = nullptr;
    }
    if (g_PlayerSelectTitleFont) {
        g_PlayerSelectTitleFont->Release();
        g_PlayerSelectTitleFont = nullptr;
    }
    if (g_PlayerSelectHintFont) {
        g_PlayerSelectHintFont->Release();
        g_PlayerSelectHintFont = nullptr;
    }
}

void NotifyPlayerSelectDeviceLost() {
    if (g_PlayerSelectFont) g_PlayerSelectFont->OnLostDevice();
    if (g_PlayerSelectTitleFont) g_PlayerSelectTitleFont->OnLostDevice();
    if (g_PlayerSelectHintFont) g_PlayerSelectHintFont->OnLostDevice();
}

void NotifyPlayerSelectDeviceReset() {
    if (g_PlayerSelectFont && FAILED(g_PlayerSelectFont->OnResetDevice())) {
        g_PlayerSelectFont->Release();
        g_PlayerSelectFont = nullptr;
    }
    if (g_PlayerSelectTitleFont && FAILED(g_PlayerSelectTitleFont->OnResetDevice())) {
        g_PlayerSelectTitleFont->Release();
        g_PlayerSelectTitleFont = nullptr;
    }
    if (g_PlayerSelectHintFont && FAILED(g_PlayerSelectHintFont->OnResetDevice())) {
        g_PlayerSelectHintFont->Release();
        g_PlayerSelectHintFont = nullptr;
    }
    LoadPlayerSelectFont();
}

// ---------------------------------------------------------------------------
// Layout helpers
// ---------------------------------------------------------------------------

static void GetPortraitLayout(int index, float& outX, float& outY, float& outSize) {
    outSize = PORTRAIT_SIZE;
    const float totalW = (ROSTER_COUNT * PORTRAIT_SIZE) + ((ROSTER_COUNT - 1) * PORTRAIT_GAP);
    const float startX = ((float)SCREEN_WIDTH - totalW) * 0.5f;
    outX = startX + (float)index * (PORTRAIT_SIZE + PORTRAIT_GAP);
    outY = PORTRAIT_TOP;
}

static void UpdatePortraitHitboxes() {
    for (int i = 0; i < ROSTER_COUNT; i++) {
        float x = 0.0f;
        float y = 0.0f;
        float size = 0.0f;
        GetPortraitLayout(i, x, y, size);
        g_PortraitHits[i] = {
            (LONG)x,
            (LONG)y,
            (LONG)(x + size),
            (LONG)(y + size)
        };
    }
}

static void ConfirmCurrentHighlight(int& choice) {
    g_SoundManager.PlaySelectionSound();

    if (g_SelectPhase == 0) {
        g_PendingP1 = g_Roster[g_HighlightIndex].id;
        g_SelectPhase = 1;
        g_LastHoveredPortrait = -1;
    }
    else if (g_SelectPhase == 1) {
        g_SelectedP1 = g_PendingP1;
        g_SelectedP2 = g_Roster[g_HighlightIndex].id;
        g_SelectPhase = 2;
        g_ModeHighlight = 0;
        g_LastHoveredPortrait = -1;
        g_LastHoveredModeOption = -1;
    }
    else {
        g_SelectedBattleMode = (g_ModeHighlight == 0) ? BattleMode::Battle : BattleMode::Tutorial;
        choice = 1;
    }
}

// ---------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------

static void DrawModeOption(int index, bool highlighted) {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    GetModeOptionLayout(index, x, y, w, h);

    const BattleMode mode = (index == 0) ? BattleMode::Battle : BattleMode::Tutorial;
    DrawDebugRect(spriteBrush, x, y, w, h, D3DCOLOR_ARGB(200, 32, 32, 32));

    const float frame = PORTRAIT_FRAME_THICKNESS;
    const D3DCOLOR frameColor = highlighted ? PLAYER_SELECT_FRAME_COLOR : PLAYER_SELECT_FRAME_IDLE;
    DrawDebugRect(spriteBrush, x - frame, y - frame, w + frame * 2.0f, frame, frameColor);
    DrawDebugRect(spriteBrush, x - frame, y + h, w + frame * 2.0f, frame, frameColor);
    DrawDebugRect(spriteBrush, x - frame, y - frame, frame, h + frame * 2.0f, frameColor);
    DrawDebugRect(spriteBrush, x + w, y - frame, frame, h + frame * 2.0f, frameColor);

    if (g_PlayerSelectFont) {
        RECT labelRect = {
            (LONG)x,
            (LONG)(y + 18.0f),
            (LONG)(x + w),
            (LONG)(y + h)
        };
        const D3DCOLOR textColor = highlighted
            ? PLAYER_SELECT_COLOR_SELECTED
            : PLAYER_SELECT_COLOR_NORMAL;
        g_PlayerSelectFont->DrawTextA(
            spriteBrush,
            GetBattleModeLabel(mode),
            -1,
            &labelRect,
            DT_CENTER | DT_TOP | DT_SINGLELINE,
            textColor);
    }
}

static void DrawModeSelectUi() {
    if (!spriteBrush) return;

    if (g_PlayerSelectTitleFont) {
        RECT titleRect = {
            0,
            PLAYER_SELECT_TITLE_TOP,
            SCREEN_WIDTH,
            PLAYER_SELECT_TITLE_BOTTOM
        };
        g_PlayerSelectTitleFont->DrawTextA(
            spriteBrush,
            "SELECT MODE",
            -1,
            &titleRect,
            DT_CENTER | DT_TOP,
            PLAYER_SELECT_COLOR_NORMAL);
    }

    if (g_PlayerSelectFont) {
        char vsLine[96];
        sprintf_s(
            vsLine,
            "%s  VS  %s",
            GetCharacterDisplayName(g_SelectedP1),
            GetCharacterDisplayName(g_SelectedP2));
        RECT vsRect = {
            0,
            PLAYER_SELECT_VS_TOP,
            SCREEN_WIDTH,
            PLAYER_SELECT_VS_BOTTOM
        };
        g_PlayerSelectFont->DrawTextA(
            spriteBrush,
            vsLine,
            -1,
            &vsRect,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE,
            PLAYER_SELECT_COLOR_SELECTED);
    }

    DrawModeOption(0, g_ModeHighlight == 0);
    DrawModeOption(1, g_ModeHighlight == 1);

    if (g_PlayerSelectHintFont) {
        RECT hintRect = {
            PLAYER_SELECT_HINT_SIDE_MARGIN,
            PLAYER_SELECT_HINT_TOP,
            SCREEN_WIDTH - PLAYER_SELECT_HINT_SIDE_MARGIN,
            PLAYER_SELECT_HINT_BOTTOM
        };
        g_PlayerSelectHintFont->DrawTextA(
            spriteBrush,
            "Left/Right or A/D: change mode    Enter / Click: Confirm    Esc: back",
            -1,
            &hintRect,
            DT_CENTER | DT_TOP | DT_SINGLELINE,
            PLAYER_SELECT_COLOR_DIM);
    }
}

static void DrawPortrait(int index, bool highlighted) {
    float x = 0.0f;
    float y = 0.0f;
    float size = 0.0f;
    GetPortraitLayout(index, x, y, size);

    LPDIRECT3DTEXTURE9 tex = g_Roster[index].texture;
    if (tex) {
        D3DSURFACE_DESC desc;
        tex->GetLevelDesc(0, &desc);
        const float scaleX = size / (float)desc.Width;
        const float scaleY = size / (float)desc.Height;

        D3DXMATRIX matScale, matTrans, matFinal;
        D3DXMatrixScaling(&matScale, scaleX, scaleY, 1.0f);
        D3DXMatrixTranslation(&matTrans, x, y, 0.0f);
        matFinal = matScale * matTrans;
        spriteBrush->SetTransform(&matFinal);

        D3DXVECTOR3 zeroPos(0.0f, 0.0f, 0.0f);
        const D3DCOLOR tint = highlighted
            ? D3DCOLOR_XRGB(255, 255, 255)
            : D3DCOLOR_XRGB(180, 180, 180);
        spriteBrush->Draw(tex, NULL, NULL, &zeroPos, tint);

        D3DXMATRIX matIdentity;
        D3DXMatrixIdentity(&matIdentity);
        spriteBrush->SetTransform(&matIdentity);
    }
    else {
        DrawDebugRect(spriteBrush, x, y, size, size, D3DCOLOR_ARGB(255, 40, 40, 40));
    }

    const float frame = PORTRAIT_FRAME_THICKNESS;
    const D3DCOLOR frameColor = highlighted ? PLAYER_SELECT_FRAME_COLOR : PLAYER_SELECT_FRAME_IDLE;
    DrawDebugRect(spriteBrush, x - frame, y - frame, size + frame * 2.0f, frame, frameColor);
    DrawDebugRect(spriteBrush, x - frame, y + size, size + frame * 2.0f, frame, frameColor);
    DrawDebugRect(spriteBrush, x - frame, y - frame, frame, size + frame * 2.0f, frameColor);
    DrawDebugRect(spriteBrush, x + size, y - frame, frame, size + frame * 2.0f, frameColor);

    if (g_PlayerSelectFont) {
        RECT nameRect = {
            (LONG)x,
            (LONG)(y + size + PORTRAIT_NAME_GAP),
            (LONG)(x + size),
            (LONG)(y + size + PORTRAIT_NAME_GAP + PLAYER_SELECT_LABEL_HEIGHT)
        };
        const D3DCOLOR nameColor = highlighted
            ? PLAYER_SELECT_COLOR_SELECTED
            : PLAYER_SELECT_COLOR_NORMAL;
        g_PlayerSelectFont->DrawTextA(
            spriteBrush,
            GetCharacterDisplayName(g_Roster[index].id),
            -1,
            &nameRect,
            DT_CENTER | DT_TOP | DT_SINGLELINE,
            nameColor);
    }
}

static void DrawPlayerSelectUi() {
    if (!spriteBrush) return;

    if (g_SelectPhase == 2) {
        DrawModeSelectUi();
        return;
    }

    UpdatePortraitHitboxes();

    if (g_PlayerSelectTitleFont) {
        RECT titleRect = {
            0,
            PLAYER_SELECT_TITLE_TOP,
            SCREEN_WIDTH,
            PLAYER_SELECT_TITLE_BOTTOM
        };
        const char* title = (g_SelectPhase == 0) ? "P1 SELECT" : "P2 SELECT";
        g_PlayerSelectTitleFont->DrawTextA(
            spriteBrush,
            title,
            -1,
            &titleRect,
            DT_CENTER | DT_TOP,
            PLAYER_SELECT_COLOR_NORMAL);
    }

    // P1 select: show "??? VS ???". P2 select: show "MAKOTO VS NARUKAMI".
    if (g_PlayerSelectFont) {
        char vsLine[96];
        if (g_SelectPhase == 0) {
            sprintf_s(
                vsLine,
                "%s  VS  ???",
                GetCharacterDisplayName(g_Roster[g_HighlightIndex].id));
        }
        else {
            sprintf_s(
                vsLine,
                "%s  VS  %s",
                GetCharacterDisplayName(g_PendingP1),
                GetCharacterDisplayName(g_Roster[g_HighlightIndex].id));
        }

        RECT vsRect = {
            0,
            PLAYER_SELECT_VS_TOP,
            SCREEN_WIDTH,
            PLAYER_SELECT_VS_BOTTOM
        };
        g_PlayerSelectFont->DrawTextA(
            spriteBrush,
            vsLine,
            -1,
            &vsRect,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE,
            PLAYER_SELECT_COLOR_SELECTED);
    }

    for (int i = 0; i < ROSTER_COUNT; i++) {
        DrawPortrait(i, i == g_HighlightIndex);
    }

    if (g_PlayerSelectHintFont) {
        RECT hintRect = {
            PLAYER_SELECT_HINT_SIDE_MARGIN,
            PLAYER_SELECT_HINT_TOP,
            SCREEN_WIDTH - PLAYER_SELECT_HINT_SIDE_MARGIN,
            PLAYER_SELECT_HINT_BOTTOM
        };
        g_PlayerSelectHintFont->DrawTextA(
            spriteBrush,
            "Left/Right or A/D: change    Enter / Click: Confirm    Esc: back",
            -1,
            &hintRect,
            DT_CENTER | DT_TOP | DT_SINGLELINE,
            PLAYER_SELECT_COLOR_DIM);
    }
}

static void RenderPlayerSelect() {
    if (!spriteBrush) return;

    spriteBrush->Begin(D3DXSPRITE_ALPHABLEND);
    DrawPlayerSelectUi();
    spriteBrush->End();
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

static void UpdateModeSelectHoverFromCursor() {
    POINT cursorPt = {};
    int hovered = -1;
    if (GetGameCursorPos(cursorPt)) {
        for (int i = 0; i < 2; i++) {
            if (PtInRect(&g_ModeOptionHits[i], cursorPt)) {
                hovered = i;
                g_ModeHighlight = i;
                break;
            }
        }
    }

    if (hovered != g_LastHoveredModeOption) {
        if (hovered >= 0) {
            g_SoundManager.PlaySelectionSound();
        }
        g_LastHoveredModeOption = hovered;
    }
}

static void UpdatePlayerSelectHoverFromCursor() {
    POINT cursorPt = {};
    int hovered = -1;
    if (GetGameCursorPos(cursorPt)) {
        for (int i = 0; i < ROSTER_COUNT; i++) {
            if (PtInRect(&g_PortraitHits[i], cursorPt)) {
                hovered = i;
                g_HighlightIndex = i;
                break;
            }
        }
    }

    if (hovered != g_LastHoveredPortrait) {
        if (hovered >= 0) {
            g_SoundManager.PlaySelectionSound();
        }
        g_LastHoveredPortrait = hovered;
    }
}

static void UpdatePlayerSelectInput(int& choice) {
    if (g_SelectPhase == 2) {
        UpdateModeOptionHitboxes();
        UpdateModeSelectHoverFromCursor();

        bool leftPressed = IsUiKeyDown(DIK_LEFT) || IsUiKeyDown(DIK_A);
        bool rightPressed = IsUiKeyDown(DIK_RIGHT) || IsUiKeyDown(DIK_D);
        bool enterPressed = IsUiKeyDown(DIK_RETURN);
        bool backPressed = IsUiKeyDown(DIK_BACK) || IsUiKeyDown(DIK_ESCAPE);
        bool clickPressed = g_WindowHasFocus && ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);

        POINT cursorPt = {};
        bool hasCursor = GetGameCursorPos(cursorPt);

        if (leftPressed && !g_PlayerSelectLeftHeld) {
            g_ModeHighlight = (g_ModeHighlight - 1 + 2) % 2;
            g_SoundManager.PlaySelectionSound();
        }
        if (rightPressed && !g_PlayerSelectRightHeld) {
            g_ModeHighlight = (g_ModeHighlight + 1) % 2;
            g_SoundManager.PlaySelectionSound();
        }
        g_PlayerSelectLeftHeld = leftPressed;
        g_PlayerSelectRightHeld = rightPressed;

        if (enterPressed && !g_PlayerSelectEnterHeld) {
            ConfirmCurrentHighlight(choice);
        }
        g_PlayerSelectEnterHeld = enterPressed;

        if (backPressed && !g_PlayerSelectBackHeld) {
            g_SoundManager.PlaySelectionSound();
            g_SelectPhase = 1;
            g_LastHoveredModeOption = -1;
            for (int i = 0; i < ROSTER_COUNT; i++) {
                if (g_Roster[i].id == g_SelectedP2) {
                    g_HighlightIndex = i;
                    break;
                }
            }
        }
        g_PlayerSelectBackHeld = backPressed;

        if (clickPressed && !g_PlayerSelectClickHeld && hasCursor) {
            for (int i = 0; i < 2; i++) {
                if (PtInRect(&g_ModeOptionHits[i], cursorPt)) {
                    g_ModeHighlight = i;
                    ConfirmCurrentHighlight(choice);
                    break;
                }
            }
        }
        g_PlayerSelectClickHeld = clickPressed;
        return;
    }

    UpdatePortraitHitboxes();
    UpdatePlayerSelectHoverFromCursor();

    bool leftPressed = IsUiKeyDown(DIK_LEFT) || IsUiKeyDown(DIK_A);
    bool rightPressed = IsUiKeyDown(DIK_RIGHT) || IsUiKeyDown(DIK_D);
    bool enterPressed = IsUiKeyDown(DIK_RETURN);
    bool backPressed = IsUiKeyDown(DIK_BACK) || IsUiKeyDown(DIK_ESCAPE);
    bool clickPressed = g_WindowHasFocus && ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);

    POINT cursorPt = {};
    bool hasCursor = GetGameCursorPos(cursorPt);

    if (leftPressed && !g_PlayerSelectLeftHeld) {
        g_HighlightIndex = (g_HighlightIndex - 1 + ROSTER_COUNT) % ROSTER_COUNT;
        g_SoundManager.PlaySelectionSound();
    }
    if (rightPressed && !g_PlayerSelectRightHeld) {
        g_HighlightIndex = (g_HighlightIndex + 1) % ROSTER_COUNT;
        g_SoundManager.PlaySelectionSound();
    }
    g_PlayerSelectLeftHeld = leftPressed;
    g_PlayerSelectRightHeld = rightPressed;

    if (enterPressed && !g_PlayerSelectEnterHeld) {
        ConfirmCurrentHighlight(choice);
    }
    g_PlayerSelectEnterHeld = enterPressed;

    if (backPressed && !g_PlayerSelectBackHeld) {
        g_SoundManager.PlaySelectionSound();
        if (g_SelectPhase == 1) {
            g_SelectPhase = 0;
            g_LastHoveredPortrait = -1;
            // Restore highlight to the pending P1 character.
            for (int i = 0; i < ROSTER_COUNT; i++) {
                if (g_Roster[i].id == g_PendingP1) {
                    g_HighlightIndex = i;
                    break;
                }
            }
        }
        else {
            choice = 2;
        }
    }
    g_PlayerSelectBackHeld = backPressed;

    if (clickPressed && !g_PlayerSelectClickHeld && hasCursor) {
        for (int i = 0; i < ROSTER_COUNT; i++) {
            if (PtInRect(&g_PortraitHits[i], cursorPt)) {
                g_HighlightIndex = i;
                ConfirmCurrentHighlight(choice);
                break;
            }
        }
    }
    g_PlayerSelectClickHeld = clickPressed;
}

void ResetPlayerSelectInputState() {
    g_PlayerSelectLeftHeld = IsUiKeyDown(DIK_LEFT) || IsUiKeyDown(DIK_A);
    g_PlayerSelectRightHeld = IsUiKeyDown(DIK_RIGHT) || IsUiKeyDown(DIK_D);
    g_PlayerSelectEnterHeld = IsUiKeyDown(DIK_RETURN);
    g_PlayerSelectBackHeld = IsUiKeyDown(DIK_BACK) || IsUiKeyDown(DIK_ESCAPE);
    g_PlayerSelectClickHeld = g_WindowHasFocus && ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);
    g_LastHoveredPortrait = -1;
    g_LastHoveredModeOption = -1;
    g_SelectPhase = 0;
    g_ModeHighlight = 0;
    g_HighlightIndex = 0;
}

void playerSelectScreen(int& choice) {
    if (!g_PlayerSelectFont || !g_PlayerSelectTitleFont || !g_PlayerSelectHintFont) {
        LoadPlayerSelectFont();
    }

    UpdatePlayerSelectInput(choice);
    RenderPlayerSelect();
}
