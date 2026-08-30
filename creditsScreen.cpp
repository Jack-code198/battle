#include "CreditsScreen.h"
#include "Input.h"
#include "Audio.h"
#include "MainMenu.h"

static ID3DXFont* g_CreditsTitleFont = nullptr;
static ID3DXFont* g_CreditsPartFont = nullptr;
static ID3DXFont* g_CreditsNameFont = nullptr;
static ID3DXFont* g_CreditsThanksFont = nullptr;
static ID3DXFont* g_CreditsFinFont = nullptr;
static ID3DXFont* g_CreditsHintFont = nullptr;
static bool g_CreditsEscHeld = false;

static const D3DCOLOR CREDITS_TITLE_COLOR = D3DCOLOR_XRGB(255, 230, 80);
static const D3DCOLOR CREDITS_PART_COLOR = D3DCOLOR_XRGB(119, 178, 170);
static const D3DCOLOR CREDITS_NAME_COLOR = D3DCOLOR_XRGB(255, 255, 255);
static const D3DCOLOR CREDITS_HINT_COLOR = D3DCOLOR_XRGB(200, 200, 200);

static const float CREDITS_SCROLL_SPEED = 1.15f;
static const DWORD THANKS_FADE_IN_MS = 2000;
static const DWORD THANKS_HOLD_MS = 2400;
static const DWORD THANKS_FADE_OUT_MS = 2000;
static const DWORD BLACK_HOLD_MS = 500;
static const DWORD FIN_FADE_IN_MS = 2000;

struct CreditEntry {
    const char* part;
    const char* name;
};

static const CreditEntry kCreditEntries[] = {
    { "Character Systems & Implementation", "Heng Jun Zhe" },
    { "Game Logic, Collision & Physics", "Heng Jun Zhe" },
    { "Parallax Battle Backgrounds", "Heng Jun Zhe" },
    { "CPU AI & Battle Behaviour", "Lim Rui Heng" },
    { "UI Menus, HUD & Options", "Lim Rui Heng" },
    { "Audio, Rendering & DirectX", "Lim Rui Heng" },
    { "Mini Game Physics Demo", "Jonathan" },
    { "Game State Stack & Input", "Jonathan" },
    { "Battle Flow & Stage Select", "Jonathan" },
};
static const int kCreditEntryCount = (int)(sizeof(kCreditEntries) / sizeof(kCreditEntries[0]));

enum class ScrollLineKind {
    Empty,
    Title,
    Section,
    Part,
    Name
};

struct ScrollLine {
    const char* text;
    ScrollLineKind kind;
    float height;
};

static ScrollLine g_ScrollLines[64] = {};
static int g_ScrollLineCount = 0;
static float g_ScrollTotalHeight = 0.0f;

enum class CreditsPhase {
    Scrolling,
    ThanksFadeIn,
    ThanksHold,
    ThanksFadeOut,
    BlackHold,
    FinFadeIn,
    FinHold
};

static CreditsPhase g_CreditsPhase = CreditsPhase::Scrolling;
static float g_CreditsScrollY = 0.0f;
static DWORD g_CreditsPhaseStartMs = 0;

static bool CreateCreditsFont(const char* familyName, INT height, BOOL italic, ID3DXFont** outFont) {
    HRESULT hr = D3DXCreateFontA(
        g_pD3DDevice,
        height,
        0,
        FW_BOLD,
        1,
        italic,
        DEFAULT_CHARSET,
        OUT_TT_PRECIS,
        ANTIALIASED_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        familyName,
        outFont);
    return SUCCEEDED(hr) && *outFont != nullptr;
}

static bool EnsureCreditsFonts() {
    if (g_CreditsTitleFont && g_CreditsPartFont && g_CreditsNameFont
        && g_CreditsThanksFont && g_CreditsFinFont && g_CreditsHintFont) {
        return true;
    }

    if (!CreateCreditsFont(NORMAL_FONT_FAMILY, 42, FALSE, &g_CreditsTitleFont)) {
        CreateCreditsFont("Arial", 42, FALSE, &g_CreditsTitleFont);
    }
    if (!CreateCreditsFont(NORMAL_FONT_FAMILY, 28, FALSE, &g_CreditsPartFont)) {
        CreateCreditsFont("Arial", 28, FALSE, &g_CreditsPartFont);
    }
    if (!CreateCreditsFont(NORMAL_FONT_FAMILY, 32, TRUE, &g_CreditsNameFont)) {
        CreateCreditsFont("Arial", 32, TRUE, &g_CreditsNameFont);
    }
    if (!CreateCreditsFont(NORMAL_FONT_FAMILY, 56, FALSE, &g_CreditsThanksFont)) {
        CreateCreditsFont("Arial", 56, FALSE, &g_CreditsThanksFont);
    }
    if (!CreateCreditsFont(NORMAL_FONT_FAMILY, 72, FALSE, &g_CreditsFinFont)) {
        CreateCreditsFont("Arial", 72, FALSE, &g_CreditsFinFont);
    }
    if (!CreateCreditsFont(NORMAL_FONT_FAMILY, 22, FALSE, &g_CreditsHintFont)) {
        CreateCreditsFont("Arial", 22, FALSE, &g_CreditsHintFont);
    }

    return g_CreditsTitleFont && g_CreditsPartFont && g_CreditsNameFont
        && g_CreditsThanksFont && g_CreditsFinFont && g_CreditsHintFont;
}

static void AddScrollLine(const char* text, ScrollLineKind kind, float height) {
    if (g_ScrollLineCount >= (int)(sizeof(g_ScrollLines) / sizeof(g_ScrollLines[0]))) {
        return;
    }
    g_ScrollLines[g_ScrollLineCount++] = { text, kind, height };
    g_ScrollTotalHeight += height;
}

static void BuildScrollLines() {
    g_ScrollLineCount = 0;
    g_ScrollTotalHeight = 0.0f;

    AddScrollLine("", ScrollLineKind::Empty, 24.0f);
    AddScrollLine("PERSONA ARENA", ScrollLineKind::Title, 52.0f);
    AddScrollLine("END CREDITS", ScrollLineKind::Title, 52.0f);
    AddScrollLine("", ScrollLineKind::Empty, 24.0f);
    AddScrollLine("DEVELOPMENT TEAM", ScrollLineKind::Section, 36.0f);
    AddScrollLine("", ScrollLineKind::Empty, 20.0f);

    for (int i = 0; i < kCreditEntryCount; ++i) {
        AddScrollLine(kCreditEntries[i].part, ScrollLineKind::Part, 32.0f);
        AddScrollLine(kCreditEntries[i].name, ScrollLineKind::Name, 36.0f);
        AddScrollLine("", ScrollLineKind::Empty, 28.0f);
    }
}

static D3DCOLOR WithAlpha(D3DCOLOR color, BYTE alpha) {
    return (color & 0x00FFFFFF) | ((DWORD)alpha << 24);
}

static float PhaseProgress(DWORD durationMs) {
    if (durationMs == 0) {
        return 1.0f;
    }
    const DWORD elapsed = GetTickCount() - g_CreditsPhaseStartMs;
    if (elapsed >= durationMs) {
        return 1.0f;
    }
    return (float)elapsed / (float)durationMs;
}

static BYTE LerpAlpha(float t) {
    if (t <= 0.0f) return 0;
    if (t >= 1.0f) return 255;
    return (BYTE)(t * 255.0f);
}

static void BeginCreditsPhase(CreditsPhase phase) {
    g_CreditsPhase = phase;
    g_CreditsPhaseStartMs = GetTickCount();
}

static ID3DXFont* FontForScrollKind(ScrollLineKind kind) {
    switch (kind) {
    case ScrollLineKind::Title:
    case ScrollLineKind::Section:
        return g_CreditsTitleFont;
    case ScrollLineKind::Part:
        return g_CreditsPartFont;
    case ScrollLineKind::Name:
        return g_CreditsNameFont;
    default:
        return nullptr;
    }
}

static D3DCOLOR ColorForScrollKind(ScrollLineKind kind) {
    switch (kind) {
    case ScrollLineKind::Title:
        return CREDITS_TITLE_COLOR;
    case ScrollLineKind::Section:
        return CREDITS_NAME_COLOR;
    case ScrollLineKind::Part:
        return CREDITS_PART_COLOR;
    case ScrollLineKind::Name:
        return CREDITS_NAME_COLOR;
    default:
        return CREDITS_NAME_COLOR;
    }
}

static void DrawCenteredLine(ID3DXFont* font, const char* text, float y, float height, D3DCOLOR color) {
    if (!font || !text || !text[0]) {
        return;
    }

    RECT rect = {
        0,
        (LONG)y,
        SCREEN_WIDTH,
        (LONG)(y + height)
    };
    font->DrawTextA(spriteBrush, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE, color);
}

static void DrawFullBlackBackground() {
    DrawDebugRect(
        spriteBrush,
        0.0f,
        0.0f,
        (float)SCREEN_WIDTH,
        (float)SCREEN_HEIGHT,
        D3DCOLOR_ARGB(255, 0, 0, 0));
}

static void AdvanceCreditsTimeline() {
    switch (g_CreditsPhase) {
    case CreditsPhase::ThanksFadeIn:
        if (PhaseProgress(THANKS_FADE_IN_MS) >= 1.0f) {
            BeginCreditsPhase(CreditsPhase::ThanksHold);
        }
        break;
    case CreditsPhase::ThanksHold:
        if (PhaseProgress(THANKS_HOLD_MS) >= 1.0f) {
            BeginCreditsPhase(CreditsPhase::ThanksFadeOut);
        }
        break;
    case CreditsPhase::ThanksFadeOut:
        if (PhaseProgress(THANKS_FADE_OUT_MS) >= 1.0f) {
            BeginCreditsPhase(CreditsPhase::BlackHold);
        }
        break;
    case CreditsPhase::BlackHold:
        if (PhaseProgress(BLACK_HOLD_MS) >= 1.0f) {
            BeginCreditsPhase(CreditsPhase::FinFadeIn);
        }
        break;
    case CreditsPhase::FinFadeIn:
        if (PhaseProgress(FIN_FADE_IN_MS) >= 1.0f) {
            BeginCreditsPhase(CreditsPhase::FinHold);
        }
        break;
    default:
        break;
    }
}

static void DrawScrollingCredits() {
    float lineY = g_CreditsScrollY;
    for (int i = 0; i < g_ScrollLineCount; ++i) {
        const ScrollLine& line = g_ScrollLines[i];
        if (line.kind != ScrollLineKind::Empty && line.text && line.text[0]) {
            ID3DXFont* font = FontForScrollKind(line.kind);
            if (font) {
                DrawCenteredLine(font, line.text, lineY, line.height, ColorForScrollKind(line.kind));
            }
        }
        lineY += line.height;
    }

    g_CreditsScrollY -= CREDITS_SCROLL_SPEED;
    if (g_CreditsScrollY + g_ScrollTotalHeight < 0.0f) {
        BeginCreditsPhase(CreditsPhase::ThanksFadeIn);
    }
}

static void DrawThanksAndFin() {
    const int centerY = SCREEN_HEIGHT / 2;
    const int thanksTop = centerY - 48;
    const int thanksBottom = centerY + 48;
    const int finTop = centerY - 56;
    const int finBottom = centerY + 56;
    const int hintTop = SCREEN_HEIGHT - 56;
    const int hintBottom = SCREEN_HEIGHT - 16;

    DrawFullBlackBackground();

    switch (g_CreditsPhase) {
    case CreditsPhase::ThanksFadeIn: {
        const BYTE textAlpha = LerpAlpha(PhaseProgress(THANKS_FADE_IN_MS));
        DrawCenteredLine(
            g_CreditsThanksFont,
            "Thanks for playing",
            (float)thanksTop,
            (float)(thanksBottom - thanksTop),
            WithAlpha(CREDITS_NAME_COLOR, textAlpha));
        break;
    }
    case CreditsPhase::ThanksHold:
        DrawCenteredLine(
            g_CreditsThanksFont,
            "Thanks for playing",
            (float)thanksTop,
            (float)(thanksBottom - thanksTop),
            CREDITS_NAME_COLOR);
        break;
    case CreditsPhase::ThanksFadeOut: {
        const BYTE textAlpha = LerpAlpha(1.0f - PhaseProgress(THANKS_FADE_OUT_MS));
        DrawCenteredLine(
            g_CreditsThanksFont,
            "Thanks for playing",
            (float)thanksTop,
            (float)(thanksBottom - thanksTop),
            WithAlpha(CREDITS_NAME_COLOR, textAlpha));
        break;
    }
    case CreditsPhase::BlackHold:
        break;
    case CreditsPhase::FinFadeIn: {
        const BYTE textAlpha = LerpAlpha(PhaseProgress(FIN_FADE_IN_MS));
        DrawCenteredLine(
            g_CreditsFinFont,
            "fin",
            (float)finTop,
            (float)(finBottom - finTop),
            WithAlpha(CREDITS_NAME_COLOR, textAlpha));
        break;
    }
    case CreditsPhase::FinHold:
        DrawCenteredLine(
            g_CreditsFinFont,
            "fin",
            (float)finTop,
            (float)(finBottom - finTop),
            CREDITS_NAME_COLOR);
        DrawCenteredLine(
            g_CreditsHintFont,
            "Press Esc to return",
            (float)hintTop,
            (float)(hintBottom - hintTop),
            CREDITS_HINT_COLOR);
        break;
    default:
        break;
    }
}

static void DrawCreditsFrame() {
    if (!spriteBrush) {
        return;
    }

    renderMainMenuBackdrop();

    spriteBrush->Begin(D3DXSPRITE_ALPHABLEND);
    DrawDebugRect(
        spriteBrush,
        0.0f,
        0.0f,
        (float)SCREEN_WIDTH,
        (float)SCREEN_HEIGHT,
        D3DCOLOR_ARGB(150, 0, 0, 0));

    if (g_CreditsPhase == CreditsPhase::Scrolling) {
        DrawScrollingCredits();
    }
    else {
        DrawThanksAndFin();
    }

    spriteBrush->End();
}

void ResetCreditsScreen() {
    BuildScrollLines();
    g_CreditsPhase = CreditsPhase::Scrolling;
    g_CreditsScrollY = (float)SCREEN_HEIGHT;
    g_CreditsPhaseStartMs = GetTickCount();
    g_CreditsEscHeld = IsUiKeyDown(DIK_ESCAPE);
}

void creditsScreen(int& choice) {
    if (!EnsureCreditsFonts()) {
        return;
    }

    const bool escPressed = IsUiKeyDown(DIK_ESCAPE);
    if (escPressed && !g_CreditsEscHeld) {
        g_SoundManager.PlaySelectionSound();
        choice = 2;
    }
    g_CreditsEscHeld = escPressed;

    if (choice == 2) {
        return;
    }

    if (g_CreditsPhase != CreditsPhase::Scrolling) {
        AdvanceCreditsTimeline();
    }

    DrawCreditsFrame();
}
