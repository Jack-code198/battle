#include "pauseMenu.h"
#include "input.h"
#include "audio.h"
#include "game_logic.h"
#include <cstdio>

static ID3DXFont* g_PauseTitleFont = nullptr;
static ID3DXFont* g_PauseItemFont = nullptr;
static ID3DXFont* g_PauseHintFont = nullptr;
static ID3DXFont* g_PauseTableFont = nullptr;
static const char* g_LoadedPauseFontPath = nullptr;

// --- Pause menu state ---
static int g_PauseSelection = 0;

static bool g_PauseUpHeld = false;
static bool g_PauseDownHeld = false;
static bool g_PauseEnterHeld = false;
static bool g_PauseEscHeld = false;
static bool g_PauseClickHeld = false;

static const int PAUSE_OPTION_COUNT = 5;
static const int PAUSE_RESUME = 0;
static const int PAUSE_MOVELIST = 1;
static const int PAUSE_OPTIONS = 2;
static const int PAUSE_PLAYER_SELECT = 3;
static const int PAUSE_EXIT = 4;

static const char* g_PauseOptions[PAUSE_OPTION_COUNT] = {
    "Resume",
    "Move List",
    "Options",
    "Player Select",
    "Exit to Main Menu"
};

static RECT g_PauseItemRects[PAUSE_OPTION_COUNT] = {};
static int g_LastHoveredPauseOption = -1;

// --- Move list state ---
static bool g_MoveListBackHeld = false;
static bool g_MoveListEscHeld = false;
static bool g_MoveListLeftHeld = false;
static bool g_MoveListRightHeld = false;
static bool g_MoveListClickHeld = false;
static RECT g_MoveListBackRect = {};
static RECT g_MoveListPrevRect = {};
static RECT g_MoveListNextRect = {};
static int g_LastHoveredMoveListControl = -1; // 0=Back, 1=Prev, 2=Next
static int g_MoveListPage = 0;
static CharacterId g_MoveListLastCharacter = Char_Count; // forces page reset on first draw

enum MoveListControl {
    MoveListControl_None = -1,
    MoveListControl_Back = 0,
    MoveListControl_Prev = 1,
    MoveListControl_Next = 2
};

struct MoveEntry {
    const char* name;
    const char* button;
};

static const D3DCOLOR PAUSE_COLOR_NORMAL = D3DCOLOR_XRGB(255, 255, 255);
static const D3DCOLOR PAUSE_COLOR_SELECTED = D3DCOLOR_XRGB(255, 230, 80);
static const D3DCOLOR PAUSE_COLOR_MUTED = D3DCOLOR_XRGB(180, 180, 180);

// Panel is intentionally small/centered so the battle stays visible behind it.
static const float PAUSE_PANEL_WIDTH = 420.0f;
static const float PAUSE_PANEL_HEIGHT = 400.0f;
static const float PAUSE_FRAME_THICKNESS = 2.0f;
static const LONG PAUSE_TITLE_HEIGHT = 60;
static const int PAUSE_ITEM_HEIGHT = 46;
static const int PAUSE_ITEM_SPACING = 56;
static const LONG PAUSE_HINT_HEIGHT = 44;

// Move List panel is larger (more text) but still leaves the battle visible around it.
static const float MOVELIST_PANEL_WIDTH = 720.0f;
static const float MOVELIST_PANEL_HEIGHT = 620.0f;
static const LONG MOVELIST_TITLE_HEIGHT = 54;
static const LONG MOVELIST_NAME_HEIGHT = 40;
static const LONG MOVELIST_TABLE_TOP_GAP = 12;
static const int MOVELIST_ROW_HEIGHT = 36;
static const int MOVELIST_ROWS_PER_PAGE = 10;
static const LONG MOVELIST_FOOTER_BOTTOM_MARGIN = 20;
static const LONG MOVELIST_FOOTER_ROW_GAP = 14;
static const int MOVELIST_BACK_WIDTH = 140;
static const int MOVELIST_BACK_HEIGHT = 38;
static const int MOVELIST_PAGE_ARROW_WIDTH = 56;
static const int MOVELIST_PAGE_ARROW_HEIGHT = 44;

// --- Per-character move tables (button/key -> move name) ---
static const MoveEntry kMakotoMoves[] = {
    { "Movement",              "Left / Right (or A / D)" },
    { "Run",                   "Hold Shift + Move" },
    { "Jump",                  "Space" },
    { "Dash",                  "J" },
    { "Dodge",                 "Right Mouse Button" },
    { "Guard",                 "Away from foe + S (Shift = Run)" },
    { "Crouch",                "C" },
    { "Attack",                "Left Mouse Button" },
    { "Crouch Attack",         "C + Left Mouse Button" },
    { "Down Attack",           "S + Left Mouse Button" },
    { "Side Attack",           "E" },
    { "Attack Up",             "R" },
    { "Taunt",                 "T" },
    { "Summon: Orpheus",       "1" },
    { "Summon: Jack Frost",    "2" },
    { "Air Summon: Thanatos",  "3 (while jumping)" },
    { "Air Summon: Messiah",   "4 (while jumping)" },
    { "Ultimate: Thanatos Slash", "5" },
};

static const MoveEntry kJokerMoves[] = {
    { "Movement",              "Left / Right (or A / D)" },
    { "Run",                   "Hold Shift + Move" },
    { "Jump",                  "Space" },
    { "Dash",                  "J" },
    { "Dodge",                 "Right Mouse Button" },
    { "Ledge Roll",            "Shift + Right Mouse Button" },
    { "Guard",                 "Away from foe + S (Shift = Run)" },
    { "Attack",                "Left Mouse Button" },
    { "Down Attack",           "S + Left Mouse Button" },
    { "Forward Attack",        "E" },
    { "Up Attack",             "R" },
    { "Forward Smash",         "Shift + E" },
    { "Up Smash",              "Shift + R" },
    { "Down Smash",            "Shift + S + Left Mouse Button" },
    { "Taunt",                 "T" },
    { "Skill: Eiha",           "1" },
    { "Skill: Eigaon",         "2" },
    { "Neutral Special",       "3" },
    { "Ultimate: All-Out Attack", "5" },
};

static const MoveEntry kNarukamiMoves[] = {
    { "Movement",              "Left / Right (or A / D)" },
    { "Run",                   "Hold Shift + Move" },
    { "Jump",                  "Space" },
    { "Jump Attack",           "Space + Left Mouse Button" },
    { "Dash",                  "J" },
    { "Guard",                 "Away from foe + S (Shift = Run)" },
    { "Crouch",                "C" },
    { "Crouch Attack",         "C + Left Mouse Button" },
    { "Attack",                "Left Mouse Button" },
    { "Lightning Flash",       "G" },
    { "Raging Lion",           "W + Left Mouse Button" },
    { "Swift Strike",          "S + Left Mouse Button" },
    { "Big Gamble",            "E" },
    { "Cross Slash",           "R" },
    { "Taunt",                 "T" },
    { "Summon: Zio",           "1" },
    { "Summon: Ziodyne",       "2" },
    { "Persona Summon (Ground)", "3" },
    { "Persona Summon (Air)",  "4" },
    { "Ultimate: Myriad Truths", "5" },
};

static const MoveEntry kYosukeMoves[] = {
    { "Movement",              "Left / Right (or A / D)" },
    { "Run",                   "Hold Shift + Move" },
    { "Jump",                  "Space" },
    { "Dash",                  "J" },
    { "Back Dash",             "Right Mouse Button (hold away)" },
    { "Guard",                 "Away from foe + S (Shift = Run)" },
    { "Attack",                "Left Mouse Button" },
    { "Moonsault",             "E" },
    { "Flying Kunai",          "Space + E (air chord)" },
    { "Crescent Slash",        "R" },
    { "Skill: Persona Summon", "1" },
    { "Skill: Mirage Slash",   "2" },
    { "Skill: Brave Blade",    "3" },
    { "Skill: Garudyne",       "4" },
};

static const MoveEntry* GetMoveListForCharacter(CharacterId id, int& outCount) {
    switch (id) {
    case Char_Makoto:
        outCount = sizeof(kMakotoMoves) / sizeof(kMakotoMoves[0]);
        return kMakotoMoves;
    case Char_Joker:
        outCount = sizeof(kJokerMoves) / sizeof(kJokerMoves[0]);
        return kJokerMoves;
    case Char_Narukami:
        outCount = sizeof(kNarukamiMoves) / sizeof(kNarukamiMoves[0]);
        return kNarukamiMoves;
    case Char_Yosuke:
        outCount = sizeof(kYosukeMoves) / sizeof(kYosukeMoves[0]);
        return kYosukeMoves;
    default:
        outCount = 0;
        return nullptr;
    }
}

static bool CreatePauseUiFont(const char* familyName, INT height, ID3DXFont** outFont) {
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

static bool LoadOrCreatePauseUiFont(INT height, ID3DXFont** outFont) {
    if (*outFont) return true;

    if (!CreatePauseUiFont(NORMAL_FONT_FAMILY, height, outFont)) {
        if (*outFont) {
            (*outFont)->Release();
            *outFont = nullptr;
        }
        if (!CreatePauseUiFont("Arial", height, outFont)) {
            *outFont = nullptr;
            return false;
        }
    }
    return true;
}

static bool LoadPauseFont() {
    if (g_PauseTitleFont && g_PauseItemFont && g_PauseHintFont && g_PauseTableFont) {
        return true;
    }

    if (!g_LoadedPauseFontPath) {
        if (AddFontResourceExA(NORMAL_FONT_FILE, FR_PRIVATE, 0) != 0) {
            g_LoadedPauseFontPath = NORMAL_FONT_FILE;
        }
    }

    bool ok = true;
    if (!LoadOrCreatePauseUiFont(36, &g_PauseTitleFont)) ok = false;
    if (!LoadOrCreatePauseUiFont(26, &g_PauseItemFont)) ok = false;
    if (!LoadOrCreatePauseUiFont(16, &g_PauseHintFont)) ok = false;
    if (!LoadOrCreatePauseUiFont(20, &g_PauseTableFont)) ok = false;
    return ok;
}

bool LoadPauseMenuTextures() {
    return LoadPauseFont();
}

void CleanUpPauseMenuTextures() {
    if (g_PauseTitleFont) {
        g_PauseTitleFont->Release();
        g_PauseTitleFont = nullptr;
    }
    if (g_PauseItemFont) {
        g_PauseItemFont->Release();
        g_PauseItemFont = nullptr;
    }
    if (g_PauseHintFont) {
        g_PauseHintFont->Release();
        g_PauseHintFont = nullptr;
    }
    if (g_PauseTableFont) {
        g_PauseTableFont->Release();
        g_PauseTableFont = nullptr;
    }
}

void NotifyPauseMenuDeviceLost() {
    if (g_PauseTitleFont) g_PauseTitleFont->OnLostDevice();
    if (g_PauseItemFont) g_PauseItemFont->OnLostDevice();
    if (g_PauseHintFont) g_PauseHintFont->OnLostDevice();
    if (g_PauseTableFont) g_PauseTableFont->OnLostDevice();
}

void NotifyPauseMenuDeviceReset() {
    if (g_PauseTitleFont) g_PauseTitleFont->OnResetDevice();
    if (g_PauseItemFont) g_PauseItemFont->OnResetDevice();
    if (g_PauseHintFont) g_PauseHintFont->OnResetDevice();
    if (g_PauseTableFont) g_PauseTableFont->OnResetDevice();
}

static void GetPausePanelOrigin(float& left, float& top) {
    left = ((float)SCREEN_WIDTH - PAUSE_PANEL_WIDTH) * 0.5f;
    top = ((float)SCREEN_HEIGHT - PAUSE_PANEL_HEIGHT) * 0.5f;
}

static void UpdatePauseItemRects() {
    float panelLeft = 0.0f, panelTop = 0.0f;
    GetPausePanelOrigin(panelLeft, panelTop);

    const LONG listTop = (LONG)panelTop + PAUSE_TITLE_HEIGHT;
    const LONG left = (LONG)panelLeft + 30;
    const LONG right = (LONG)(panelLeft + PAUSE_PANEL_WIDTH) - 30;

    for (int i = 0; i < PAUSE_OPTION_COUNT; i++) {
        g_PauseItemRects[i].left = left;
        g_PauseItemRects[i].top = listTop + (i * PAUSE_ITEM_SPACING);
        g_PauseItemRects[i].right = right;
        g_PauseItemRects[i].bottom = g_PauseItemRects[i].top + PAUSE_ITEM_HEIGHT;
    }
}

static void DrawPausePanelFrame(float left, float top, float width, float height) {
    // Dim the battle slightly so the panel reads clearly without hiding it.
    DrawDebugRect(spriteBrush, 0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, D3DCOLOR_ARGB(110, 0, 0, 0));

    DrawDebugRect(spriteBrush, left, top, width, height, D3DCOLOR_ARGB(235, 24, 24, 28));

    const float frame = PAUSE_FRAME_THICKNESS;
    D3DCOLOR frameColor = D3DCOLOR_ARGB(220, 255, 255, 255);
    DrawDebugRect(spriteBrush, left - frame, top - frame, width + frame * 2.0f, frame, frameColor);
    DrawDebugRect(spriteBrush, left - frame, top + height, width + frame * 2.0f, frame, frameColor);
    DrawDebugRect(spriteBrush, left - frame, top - frame, frame, height + frame * 2.0f, frameColor);
    DrawDebugRect(spriteBrush, left + width, top - frame, frame, height + frame * 2.0f, frameColor);
}

void pauseMenuScreen(int& choice) {
    if (!g_PauseTitleFont || !g_PauseItemFont || !g_PauseHintFont || !g_PauseTableFont) {
        LoadPauseFont();
    }
    if (!spriteBrush) return;

    // --- Input ---
    UpdatePauseItemRects();

    bool upPressed = (diKeys[DIK_UP] & 0x80) != 0 || (diKeys[DIK_W] & 0x80) != 0;
    bool downPressed = (diKeys[DIK_DOWN] & 0x80) != 0 || (diKeys[DIK_S] & 0x80) != 0;
    bool enterPressed = (diKeys[DIK_RETURN] & 0x80) != 0;
    bool escPressed = (diKeys[DIK_ESCAPE] & 0x80) != 0;
    bool clickPressed = g_WindowHasFocus && ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);

    POINT cursorPt = {};
    bool hasCursor = GetGameCursorPos(cursorPt);
    int hoveredOption = -1;
    if (hasCursor) {
        for (int i = 0; i < PAUSE_OPTION_COUNT; i++) {
            if (PtInRect(&g_PauseItemRects[i], cursorPt)) {
                hoveredOption = i;
                g_PauseSelection = i;
                break;
            }
        }
    }

    if (hoveredOption != g_LastHoveredPauseOption) {
        if (hoveredOption >= 0) {
            g_SoundManager.PlaySelectionSound();
        }
        g_LastHoveredPauseOption = hoveredOption;
    }

    if (upPressed && !g_PauseUpHeld) {
        g_PauseSelection = (g_PauseSelection - 1 + PAUSE_OPTION_COUNT) % PAUSE_OPTION_COUNT;
        g_SoundManager.PlaySelectionSound();
    }
    if (downPressed && !g_PauseDownHeld) {
        g_PauseSelection = (g_PauseSelection + 1) % PAUSE_OPTION_COUNT;
        g_SoundManager.PlaySelectionSound();
    }
    g_PauseUpHeld = upPressed;
    g_PauseDownHeld = downPressed;

    if (enterPressed && !g_PauseEnterHeld) {
        g_SoundManager.PlaySelectionSound();
        choice = g_PauseSelection + 1; // 1=Resume, 2=MoveList, 3=Options, 4=PlayerSelect, 5=Exit
    }
    g_PauseEnterHeld = enterPressed;

    // Esc while paused resumes, mirroring the Esc that opened the menu.
    if (escPressed && !g_PauseEscHeld) {
        g_SoundManager.PlaySelectionSound();
        choice = PAUSE_RESUME + 1;
    }
    g_PauseEscHeld = escPressed;

    if (clickPressed && !g_PauseClickHeld && hoveredOption >= 0) {
        g_PauseSelection = hoveredOption;
        g_SoundManager.PlaySelectionSound();
        choice = g_PauseSelection + 1;
    }
    g_PauseClickHeld = clickPressed;

    // --- Draw ---
    float panelLeft = 0.0f, panelTop = 0.0f;
    GetPausePanelOrigin(panelLeft, panelTop);

    spriteBrush->Begin(D3DXSPRITE_ALPHABLEND);
    DrawPausePanelFrame(panelLeft, panelTop, PAUSE_PANEL_WIDTH, PAUSE_PANEL_HEIGHT);

    if (g_PauseTitleFont) {
        RECT titleRect = {
            (LONG)panelLeft,
            (LONG)panelTop + 8,
            (LONG)(panelLeft + PAUSE_PANEL_WIDTH),
            (LONG)panelTop + PAUSE_TITLE_HEIGHT
        };
        g_PauseTitleFont->DrawTextA(spriteBrush, "PAUSED", -1, &titleRect, DT_CENTER | DT_TOP, PAUSE_COLOR_NORMAL);
    }

    if (g_PauseItemFont) {
        for (int i = 0; i < PAUSE_OPTION_COUNT; i++) {
            const D3DCOLOR textColor = (i == g_PauseSelection) ? PAUSE_COLOR_SELECTED : PAUSE_COLOR_NORMAL;
            g_PauseItemFont->DrawTextA(
                spriteBrush,
                g_PauseOptions[i],
                -1,
                &g_PauseItemRects[i],
                DT_LEFT | DT_VCENTER | DT_SINGLELINE,
                textColor);
        }
    }

    if (g_PauseHintFont) {
        RECT hintLine1 = {
            (LONG)panelLeft + 20,
            (LONG)(panelTop + PAUSE_PANEL_HEIGHT) - PAUSE_HINT_HEIGHT,
            (LONG)(panelLeft + PAUSE_PANEL_WIDTH) - 20,
            (LONG)(panelTop + PAUSE_PANEL_HEIGHT) - PAUSE_HINT_HEIGHT + 20
        };
        RECT hintLine2 = {
            hintLine1.left,
            hintLine1.bottom,
            hintLine1.right,
            (LONG)(panelTop + PAUSE_PANEL_HEIGHT) - 6
        };
        g_PauseHintFont->DrawTextA(
            spriteBrush,
            "Up/Down / Mouse: select",
            -1,
            &hintLine1,
            DT_CENTER | DT_TOP | DT_SINGLELINE,
            PAUSE_COLOR_MUTED);
        g_PauseHintFont->DrawTextA(
            spriteBrush,
            "Enter: confirm    Esc: resume",
            -1,
            &hintLine2,
            DT_CENTER | DT_TOP | DT_SINGLELINE,
            PAUSE_COLOR_MUTED);
    }

    spriteBrush->End();
}

void ResetPauseMenuInputState() {
    g_PauseUpHeld = (diKeys[DIK_UP] & 0x80) != 0 || (diKeys[DIK_W] & 0x80) != 0;
    g_PauseDownHeld = (diKeys[DIK_DOWN] & 0x80) != 0 || (diKeys[DIK_S] & 0x80) != 0;
    g_PauseEnterHeld = (diKeys[DIK_RETURN] & 0x80) != 0;
    g_PauseEscHeld = (diKeys[DIK_ESCAPE] & 0x80) != 0;
    g_PauseClickHeld = g_WindowHasFocus && ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);
    g_PauseSelection = 0;
    g_LastHoveredPauseOption = -1;
}

// --- Move list ---

static void GetMoveListPanelOrigin(float& left, float& top) {
    left = ((float)SCREEN_WIDTH - MOVELIST_PANEL_WIDTH) * 0.5f;
    top = ((float)SCREEN_HEIGHT - MOVELIST_PANEL_HEIGHT) * 0.5f;
}

static int GetMoveListPageCount(int totalMoves) {
    if (totalMoves <= 0) return 1;
    return (totalMoves + MOVELIST_ROWS_PER_PAGE - 1) / MOVELIST_ROWS_PER_PAGE;
}

// Two stacked footer rows (page indicator/arrows, then Back), bottom-anchored
// with a fixed gap so they never overlap regardless of panel height.
static void GetMoveListFooterRows(float panelTop, RECT& outPageRow, RECT& outBackRow) {
    const LONG panelBottom = (LONG)(panelTop + MOVELIST_PANEL_HEIGHT);

    outBackRow.bottom = panelBottom - MOVELIST_FOOTER_BOTTOM_MARGIN;
    outBackRow.top = outBackRow.bottom - MOVELIST_BACK_HEIGHT;

    outPageRow.bottom = outBackRow.top - MOVELIST_FOOTER_ROW_GAP;
    outPageRow.top = outPageRow.bottom - MOVELIST_PAGE_ARROW_HEIGHT;
}

static void UpdateMoveListControlRects() {
    float panelLeft = 0.0f, panelTop = 0.0f;
    GetMoveListPanelOrigin(panelLeft, panelTop);

    RECT pageRow = {}, backRow = {};
    GetMoveListFooterRows(panelTop, pageRow, backRow);
    const float centerX = panelLeft + MOVELIST_PANEL_WIDTH * 0.5f;

    // Back button, bottom center.
    g_MoveListBackRect.left = (LONG)(centerX - MOVELIST_BACK_WIDTH * 0.5f);
    g_MoveListBackRect.right = (LONG)(centerX + MOVELIST_BACK_WIDTH * 0.5f);
    g_MoveListBackRect.top = backRow.top;
    g_MoveListBackRect.bottom = backRow.bottom;

    // Prev/Next arrows flank the page indicator, above the Back button.
    g_MoveListPrevRect.left = (LONG)panelLeft + 30;
    g_MoveListPrevRect.right = g_MoveListPrevRect.left + MOVELIST_PAGE_ARROW_WIDTH;
    g_MoveListPrevRect.top = pageRow.top;
    g_MoveListPrevRect.bottom = pageRow.bottom;

    g_MoveListNextRect.right = (LONG)(panelLeft + MOVELIST_PANEL_WIDTH) - 30;
    g_MoveListNextRect.left = g_MoveListNextRect.right - MOVELIST_PAGE_ARROW_WIDTH;
    g_MoveListNextRect.top = pageRow.top;
    g_MoveListNextRect.bottom = pageRow.bottom;
}

// Measures single-line text width using the font's own metrics (DT_CALCRECT
// draws nothing — it only recomputes the rect to fit the text).
static int MeasureTextWidth(ID3DXFont* font, const char* text, const RECT& cell) {
    if (!font || !text) return 0;
    RECT measureRect = cell;
    font->DrawTextA(nullptr, text, -1, &measureRect, DT_LEFT | DT_SINGLELINE | DT_CALCRECT | DT_NOCLIP, 0);
    return measureRect.right - measureRect.left;
}

// Ping-pong scroll offset (DBZ Dokkan-style leader-skill marquee): pause at
// the start, slide to reveal the end, pause there, slide back. Driven by
// wall-clock time so it animates smoothly regardless of the FPS cap, and
// `phaseOffsetMs` lets rows stagger instead of all sliding in lockstep.
static float GetMarqueeOffset(float maxOffset, DWORD nowMs, DWORD phaseOffsetMs) {
    if (maxOffset <= 0.0f) return 0.0f;

    const DWORD holdMs = 900;
    const float pixelsPerSecond = 55.0f;
    DWORD travelMs = (DWORD)((maxOffset / pixelsPerSecond) * 1000.0f);
    if (travelMs < 400) travelMs = 400;

    const DWORD cycleMs = holdMs * 2 + travelMs * 2;
    DWORD t = (nowMs + phaseOffsetMs) % cycleMs;

    if (t < holdMs) return 0.0f;
    t -= holdMs;
    if (t < travelMs) return maxOffset * ((float)t / (float)travelMs);
    t -= travelMs;
    if (t < holdMs) return maxOffset;
    t -= holdMs;
    float frac = (float)t / (float)travelMs;
    if (frac > 1.0f) frac = 1.0f;
    return maxOffset * (1.0f - frac);
}

// Draws text inside `cell`. If it fits, draws normally (respecting `align`).
// If it overflows, scissor-clips to `cell` and slides the text left/right so
// the full string is readable over time instead of being cut off mid-word.
static void DrawMarqueeCell(ID3DXFont* font, const char* text, const RECT& cell, DWORD align, D3DCOLOR color, DWORD nowMs, DWORD phaseOffsetMs) {
    if (!font || !spriteBrush || !text) return;

    const int cellWidth = cell.right - cell.left;
    const int textWidth = MeasureTextWidth(font, text, cell);

    if (textWidth <= cellWidth) {
        RECT drawRect = cell;
        font->DrawTextA(spriteBrush, text, -1, &drawRect, align | DT_VCENTER | DT_SINGLELINE, color);
        return;
    }

    const float maxOffset = (float)(textWidth - cellWidth);
    const float offset = GetMarqueeOffset(maxOffset, nowMs, phaseOffsetMs);

    // Render-state/scissor changes should happen between batches, not mid-batch.
    spriteBrush->End();

    RECT scissor = cell;
    g_pD3DDevice->SetScissorRect(&scissor);
    g_pD3DDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);

    spriteBrush->Begin(D3DXSPRITE_ALPHABLEND);
    RECT slideRect = cell;
    slideRect.left = cell.left - (LONG)offset;
    slideRect.right = slideRect.left + textWidth + 20;
    font->DrawTextA(spriteBrush, text, -1, &slideRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP, color);
    spriteBrush->End();

    g_pD3DDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    spriteBrush->Begin(D3DXSPRITE_ALPHABLEND); // hand the batch back open, as the caller expects
}

static void DrawMoveListTable(float panelLeft, float panelTop, const MoveEntry* entries, int totalMoves, int page) {
    if (!g_PauseTableFont || !entries || totalMoves <= 0) return;

    const LONG tableTop = (LONG)panelTop + MOVELIST_TITLE_HEIGHT + MOVELIST_NAME_HEIGHT + MOVELIST_TABLE_TOP_GAP;
    const LONG nameColLeft = (LONG)panelLeft + 40;
    const LONG nameColRight = (LONG)(panelLeft + MOVELIST_PANEL_WIDTH * 0.52f);
    const LONG buttonColLeft = nameColRight + 20;
    const LONG buttonColRight = (LONG)(panelLeft + MOVELIST_PANEL_WIDTH) - 40;

    const DWORD nowMs = GetTickCount();
    const int startIndex = page * MOVELIST_ROWS_PER_PAGE;
    const int endIndex = min(startIndex + MOVELIST_ROWS_PER_PAGE, totalMoves);

    for (int i = startIndex; i < endIndex; i++) {
        const int row = i - startIndex;
        const LONG rowTop = tableTop + (row * MOVELIST_ROW_HEIGHT);
        const LONG rowBottom = rowTop + MOVELIST_ROW_HEIGHT;
        const DWORD phaseOffsetMs = (DWORD)row * 200;

        // No visible grid — cells are just aligned text columns.
        RECT nameCell = { nameColLeft, rowTop, nameColRight, rowBottom };
        RECT buttonCell = { buttonColLeft, rowTop, buttonColRight, rowBottom };

        DrawMarqueeCell(g_PauseTableFont, entries[i].name, nameCell, DT_LEFT, PAUSE_COLOR_NORMAL, nowMs, phaseOffsetMs);
        DrawMarqueeCell(g_PauseTableFont, entries[i].button, buttonCell, DT_RIGHT, PAUSE_COLOR_MUTED, nowMs, phaseOffsetMs);
    }
}

void moveListScreen(int& choice, CharacterId activeCharacter) {
    if (!g_PauseTitleFont || !g_PauseItemFont || !g_PauseHintFont || !g_PauseTableFont) {
        LoadPauseFont();
    }
    if (!spriteBrush) return;

    // Character changed (e.g. re-entered on a different fighter) — reset paging.
    if (activeCharacter != g_MoveListLastCharacter) {
        g_MoveListLastCharacter = activeCharacter;
        g_MoveListPage = 0;
    }

    int totalMoves = 0;
    const MoveEntry* entries = GetMoveListForCharacter(activeCharacter, totalMoves);
    const int pageCount = GetMoveListPageCount(totalMoves);
    if (g_MoveListPage >= pageCount) g_MoveListPage = pageCount - 1;
    if (g_MoveListPage < 0) g_MoveListPage = 0;

    // --- Input ---
    UpdateMoveListControlRects();

    bool backPressed = (diKeys[DIK_BACK] & 0x80) != 0;
    bool escPressed = (diKeys[DIK_ESCAPE] & 0x80) != 0;
    bool leftPressed = (diKeys[DIK_LEFT] & 0x80) != 0 || (diKeys[DIK_A] & 0x80) != 0;
    bool rightPressed = (diKeys[DIK_RIGHT] & 0x80) != 0 || (diKeys[DIK_D] & 0x80) != 0;
    bool clickPressed = g_WindowHasFocus && ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);

    POINT cursorPt = {};
    bool hasCursor = GetGameCursorPos(cursorPt);
    int hoveredControl = MoveListControl_None;
    if (hasCursor) {
        if (PtInRect(&g_MoveListBackRect, cursorPt)) hoveredControl = MoveListControl_Back;
        else if (PtInRect(&g_MoveListPrevRect, cursorPt)) hoveredControl = MoveListControl_Prev;
        else if (PtInRect(&g_MoveListNextRect, cursorPt)) hoveredControl = MoveListControl_Next;
    }

    if (hoveredControl != g_LastHoveredMoveListControl) {
        if (hoveredControl != MoveListControl_None) {
            g_SoundManager.PlaySelectionSound();
        }
        g_LastHoveredMoveListControl = hoveredControl;
    }

    if ((backPressed && !g_MoveListBackHeld) || (escPressed && !g_MoveListEscHeld)) {
        g_SoundManager.PlaySelectionSound();
        choice = 2;
    }
    g_MoveListBackHeld = backPressed;
    g_MoveListEscHeld = escPressed;

    if (leftPressed && !g_MoveListLeftHeld) {
        if (g_MoveListPage > 0) {
            g_MoveListPage--;
            g_SoundManager.PlaySelectionSound();
        }
    }
    if (rightPressed && !g_MoveListRightHeld) {
        if (g_MoveListPage < pageCount - 1) {
            g_MoveListPage++;
            g_SoundManager.PlaySelectionSound();
        }
    }
    g_MoveListLeftHeld = leftPressed;
    g_MoveListRightHeld = rightPressed;

    if (clickPressed && !g_MoveListClickHeld) {
        if (hoveredControl == MoveListControl_Back) {
            g_SoundManager.PlaySelectionSound();
            choice = 2;
        }
        else if (hoveredControl == MoveListControl_Prev && g_MoveListPage > 0) {
            g_MoveListPage--;
            g_SoundManager.PlaySelectionSound();
        }
        else if (hoveredControl == MoveListControl_Next && g_MoveListPage < pageCount - 1) {
            g_MoveListPage++;
            g_SoundManager.PlaySelectionSound();
        }
    }
    g_MoveListClickHeld = clickPressed;

    // --- Draw ---
    float panelLeft = 0.0f, panelTop = 0.0f;
    GetMoveListPanelOrigin(panelLeft, panelTop);

    spriteBrush->Begin(D3DXSPRITE_ALPHABLEND);
    DrawPausePanelFrame(panelLeft, panelTop, MOVELIST_PANEL_WIDTH, MOVELIST_PANEL_HEIGHT);

    if (g_PauseTitleFont) {
        RECT titleRect = {
            (LONG)panelLeft,
            (LONG)panelTop + 8,
            (LONG)(panelLeft + MOVELIST_PANEL_WIDTH),
            (LONG)panelTop + MOVELIST_TITLE_HEIGHT
        };
        g_PauseTitleFont->DrawTextA(spriteBrush, "MOVE LIST", -1, &titleRect, DT_CENTER | DT_TOP, PAUSE_COLOR_NORMAL);
    }

    if (g_PauseItemFont) {
        RECT nameRect = {
            (LONG)panelLeft,
            (LONG)panelTop + MOVELIST_TITLE_HEIGHT,
            (LONG)(panelLeft + MOVELIST_PANEL_WIDTH),
            (LONG)panelTop + MOVELIST_TITLE_HEIGHT + MOVELIST_NAME_HEIGHT
        };
        g_PauseItemFont->DrawTextA(
            spriteBrush,
            GetCharacterDisplayName(activeCharacter),
            -1,
            &nameRect,
            DT_CENTER | DT_TOP,
            PAUSE_COLOR_SELECTED);
    }

    DrawMoveListTable(panelLeft, panelTop, entries, totalMoves, g_MoveListPage);

    if (g_PauseHintFont) {
        char pageBuffer[32];
        sprintf_s(pageBuffer, "Page %d / %d", g_MoveListPage + 1, pageCount);

        RECT pageRowRect = {}, backRowRect = {};
        GetMoveListFooterRows(panelTop, pageRowRect, backRowRect);
        RECT pageRect = {
            (LONG)panelLeft,
            pageRowRect.top,
            (LONG)(panelLeft + MOVELIST_PANEL_WIDTH),
            pageRowRect.bottom
        };
        g_PauseHintFont->DrawTextA(spriteBrush, pageBuffer, -1, &pageRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE, PAUSE_COLOR_MUTED);
    }

    if (g_PauseItemFont) {
        const bool hasPrev = g_MoveListPage > 0;
        const bool hasNext = g_MoveListPage < pageCount - 1;
        const D3DCOLOR prevColor = !hasPrev
            ? D3DCOLOR_XRGB(90, 90, 90)
            : (hoveredControl == MoveListControl_Prev ? PAUSE_COLOR_SELECTED : PAUSE_COLOR_NORMAL);
        const D3DCOLOR nextColor = !hasNext
            ? D3DCOLOR_XRGB(90, 90, 90)
            : (hoveredControl == MoveListControl_Next ? PAUSE_COLOR_SELECTED : PAUSE_COLOR_NORMAL);

        g_PauseItemFont->DrawTextA(spriteBrush, "<", -1, &g_MoveListPrevRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE, prevColor);
        g_PauseItemFont->DrawTextA(spriteBrush, ">", -1, &g_MoveListNextRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE, nextColor);
    }

    if (g_PauseItemFont) {
        const D3DCOLOR backColor = (hoveredControl == MoveListControl_Back) ? PAUSE_COLOR_SELECTED : PAUSE_COLOR_NORMAL;
        g_PauseItemFont->DrawTextA(
            spriteBrush,
            "Back",
            -1,
            &g_MoveListBackRect,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE,
            backColor);
    }

    spriteBrush->End();
}

void ResetMoveListInputState() {
    g_MoveListBackHeld = (diKeys[DIK_BACK] & 0x80) != 0;
    g_MoveListEscHeld = (diKeys[DIK_ESCAPE] & 0x80) != 0;
    g_MoveListLeftHeld = (diKeys[DIK_LEFT] & 0x80) != 0 || (diKeys[DIK_A] & 0x80) != 0;
    g_MoveListRightHeld = (diKeys[DIK_RIGHT] & 0x80) != 0 || (diKeys[DIK_D] & 0x80) != 0;
    g_MoveListClickHeld = g_WindowHasFocus && ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);
    g_LastHoveredMoveListControl = MoveListControl_None;
    g_MoveListPage = 0;
}
