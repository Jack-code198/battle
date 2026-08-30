#include "OptionMenu.h"
#include "Input.h"
#include "Audio.h"
#include <cstdio>

static ID3DXFont* g_OptionsTitleFont = nullptr;
static ID3DXFont* g_OptionsFont = nullptr;
static ID3DXFont* g_OptionsHintFont = nullptr;
static const char* g_LoadedOptionsFontPath = nullptr;

static int g_OptionsSelection = 0;

static bool g_OptionsUpHeld = false;
static bool g_OptionsDownHeld = false;
static bool g_OptionsLeftHeld = false;
static bool g_OptionsRightHeld = false;
static bool g_OptionsEnterHeld = false;
static bool g_OptionsBackHeld = false;
static bool g_OptionsClickHeld = false;

static const int OPTIONS_COUNT = 4;
static const int OPTIONS_MUSIC = 0;
static const int OPTIONS_BRIGHTNESS = 1;
static const int OPTIONS_FPS = 2;
static const int OPTIONS_BACK = 3;

static const int kFpsChoices[] = { 60, 120, 144 };
static const int kFpsChoiceCount = 3;

static RECT g_OptionsRects[OPTIONS_COUNT] = {};
static RECT g_BrightnessTrackRect = {};
static int g_LastHoveredOption = -1;
static bool g_BrightnessDragging = false;
static int g_BrightnessRepeatTimer = 0;

static const D3DCOLOR OPTIONS_COLOR_NORMAL = D3DCOLOR_XRGB(255, 255, 255);
static const D3DCOLOR OPTIONS_COLOR_SELECTED = D3DCOLOR_XRGB(255, 230, 80);

static const LONG OPTIONS_TITLE_TOP = 40;
static const LONG OPTIONS_TITLE_BOTTOM = 100;
static const int OPTIONS_ROW_WIDTH = 440;
static const int OPTIONS_ROW_HEIGHT = 48;
static const int OPTIONS_BRIGHTNESS_ROW_HEIGHT = 88;
static const int OPTIONS_ROW_SPACING = 60;
static const int OPTIONS_TOP_MARGIN = 260;
static const LONG OPTIONS_HINT_TOP = 700;
static const LONG OPTIONS_HINT_BOTTOM = 740;
static const LONG OPTIONS_HINT_SIDE_MARGIN = 40;

// Brightness: "Brightness: NN%" centered, slider on row below.
static const int BRIGHTNESS_LABEL_ROW_HEIGHT = 36;
static const int BRIGHTNESS_LABEL_SIDE_PAD = 20;
static const int BRIGHTNESS_TRACK_TOP_GAP = 18;
static const int BRIGHTNESS_TRACK_SIDE_MARGIN = 20;
static const float BRIGHTNESS_TRACK_HEIGHT = 8.0f;
static const float BRIGHTNESS_THUMB_SIZE = 16.0f;
static const int BRIGHTNESS_KEY_REPEAT_FRAMES = 6;

static bool CreateOptionsUiFont(const char* familyName, INT height, ID3DXFont** outFont) {
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

static bool LoadOrCreateOptionsUiFont(INT height, ID3DXFont** outFont) {
    if (*outFont) return true;

    if (!CreateOptionsUiFont(NORMAL_FONT_FAMILY, height, outFont)) {
        if (*outFont) {
            (*outFont)->Release();
            *outFont = nullptr;
        }
        if (!CreateOptionsUiFont("Arial", height, outFont)) {
            *outFont = nullptr;
            return false;
        }
    }
    return true;
}

static bool LoadOptionsFont() {
    if (g_OptionsTitleFont && g_OptionsFont && g_OptionsHintFont) {
        return true;
    }

    if (!g_LoadedOptionsFontPath) {
        if (AddFontResourceExA(NORMAL_FONT_FILE, FR_PRIVATE, 0) != 0) {
            g_LoadedOptionsFontPath = NORMAL_FONT_FILE;
        }
    }

    bool ok = true;
    if (!LoadOrCreateOptionsUiFont(42, &g_OptionsTitleFont)) ok = false;
    if (!LoadOrCreateOptionsUiFont(30, &g_OptionsFont)) ok = false;
    if (!LoadOrCreateOptionsUiFont(18, &g_OptionsHintFont)) ok = false;
    return ok;
}

bool LoadOptionsTextures() {
    return LoadOptionsFont();
}

void CleanUpOptionsTextures() {
    if (g_OptionsTitleFont) {
        g_OptionsTitleFont->Release();
        g_OptionsTitleFont = nullptr;
    }
    if (g_OptionsFont) {
        g_OptionsFont->Release();
        g_OptionsFont = nullptr;
    }
    if (g_OptionsHintFont) {
        g_OptionsHintFont->Release();
        g_OptionsHintFont = nullptr;
    }
}

void NotifyOptionsDeviceLost() {
    if (g_OptionsTitleFont) g_OptionsTitleFont->OnLostDevice();
    if (g_OptionsFont) g_OptionsFont->OnLostDevice();
    if (g_OptionsHintFont) g_OptionsHintFont->OnLostDevice();
}

void NotifyOptionsDeviceReset() {
    if (g_OptionsTitleFont) g_OptionsTitleFont->OnResetDevice();
    if (g_OptionsFont) g_OptionsFont->OnResetDevice();
    if (g_OptionsHintFont) g_OptionsHintFont->OnResetDevice();
}

static void UpdateOptionsRects() {
    const int centerX = SCREEN_WIDTH / 2;
    const int startY = OPTIONS_TOP_MARGIN;

    for (int i = 0; i < OPTIONS_COUNT; i++) {
        const int rowWidth = OPTIONS_ROW_WIDTH;
        const int left = centerX - rowWidth / 2;
        g_OptionsRects[i].left = left;
        g_OptionsRects[i].top = startY + (i * OPTIONS_ROW_SPACING);
        g_OptionsRects[i].right = left + rowWidth;
        g_OptionsRects[i].bottom = g_OptionsRects[i].top +
            ((i == OPTIONS_BRIGHTNESS) ? OPTIONS_BRIGHTNESS_ROW_HEIGHT : OPTIONS_ROW_HEIGHT);
    }

    const RECT& row = g_OptionsRects[OPTIONS_BRIGHTNESS];
    const LONG trackTop = row.top + BRIGHTNESS_LABEL_ROW_HEIGHT + BRIGHTNESS_TRACK_TOP_GAP;
    g_BrightnessTrackRect.left = row.left + BRIGHTNESS_TRACK_SIDE_MARGIN;
    g_BrightnessTrackRect.right = row.right - BRIGHTNESS_TRACK_SIDE_MARGIN;
    g_BrightnessTrackRect.top = trackTop;
    g_BrightnessTrackRect.bottom = trackTop + (LONG)BRIGHTNESS_TRACK_HEIGHT;
}

static float GetBrightnessFraction() {
    float fraction = (float)(g_BrightnessLevel - BRIGHTNESS_MIN) / (float)(BRIGHTNESS_MAX - BRIGHTNESS_MIN);
    if (fraction < 0.0f) fraction = 0.0f;
    if (fraction > 1.0f) fraction = 1.0f;
    return fraction;
}

static void SetBrightnessFromClientX(int clientX) {
    const int trackWidth = g_BrightnessTrackRect.right - g_BrightnessTrackRect.left;
    if (trackWidth <= 0) return;

    float fraction = (float)(clientX - g_BrightnessTrackRect.left) / (float)trackWidth;
    if (fraction < 0.0f) fraction = 0.0f;
    if (fraction > 1.0f) fraction = 1.0f;

    int value = BRIGHTNESS_MIN + (int)(fraction * (float)(BRIGHTNESS_MAX - BRIGHTNESS_MIN) + 0.5f);
    // Snap to the same step keyboard adjustment uses, for a consistent feel.
    value = ((value - BRIGHTNESS_MIN + BRIGHTNESS_STEP / 2) / BRIGHTNESS_STEP) * BRIGHTNESS_STEP + BRIGHTNESS_MIN;
    if (value < BRIGHTNESS_MIN) value = BRIGHTNESS_MIN;
    if (value > BRIGHTNESS_MAX) value = BRIGHTNESS_MAX;
    g_BrightnessLevel = value;
}

static const char* GetOptionLabel(int index, char* buffer, size_t bufferSize) {
    if (index == OPTIONS_MUSIC) {
        sprintf_s(buffer, bufferSize, "Music: %s", g_SoundManager.IsMusicMuted() ? "OFF" : "ON");
        return buffer;
    }
    if (index == OPTIONS_FPS) {
        sprintf_s(buffer, bufferSize, "FPS Limit: %d", g_TargetFPS);
        return buffer;
    }
    return "Back";
}

static void DrawBrightnessSlider(const RECT& row, bool selected, bool hovered) {
    if (!spriteBrush) return;

    const D3DCOLOR labelColor = selected ? OPTIONS_COLOR_SELECTED : OPTIONS_COLOR_NORMAL;

    // Draw track first so label / percent text stays on top and readable.
    const float trackX = (float)g_BrightnessTrackRect.left;
    const float trackY = (float)g_BrightnessTrackRect.top;
    const float trackW = (float)(g_BrightnessTrackRect.right - g_BrightnessTrackRect.left);
    const float trackH = (float)(g_BrightnessTrackRect.bottom - g_BrightnessTrackRect.top);

    DrawDebugRect(spriteBrush, trackX - 2.0f, trackY - 2.0f, trackW + 4.0f, trackH + 4.0f, D3DCOLOR_ARGB(200, 255, 255, 255));
    DrawDebugRect(spriteBrush, trackX, trackY, trackW, trackH, D3DCOLOR_ARGB(230, 16, 16, 20));

    const float fraction = GetBrightnessFraction();
    const float fillW = trackW * fraction;
    const D3DCOLOR fillColor = (selected || hovered) ? OPTIONS_COLOR_SELECTED : D3DCOLOR_XRGB(200, 200, 200);
    if (fillW > 0.0f) {
        DrawDebugRect(spriteBrush, trackX, trackY, fillW, trackH, fillColor);
    }

    const float thumbX = trackX + fillW - BRIGHTNESS_THUMB_SIZE * 0.5f;
    const float thumbY = trackY + trackH * 0.5f - BRIGHTNESS_THUMB_SIZE * 0.5f;
    DrawDebugRect(spriteBrush, thumbX, thumbY, BRIGHTNESS_THUMB_SIZE, BRIGHTNESS_THUMB_SIZE, fillColor);

    if (g_OptionsFont) {
        char labelBuffer[32];
        sprintf_s(labelBuffer, "Brightness: %d%%", g_BrightnessLevel);
        RECT labelRect = {
            row.left + BRIGHTNESS_LABEL_SIDE_PAD,
            row.top,
            row.right - BRIGHTNESS_LABEL_SIDE_PAD,
            row.top + BRIGHTNESS_LABEL_ROW_HEIGHT
        };
        g_OptionsFont->DrawTextA(
            spriteBrush,
            labelBuffer,
            -1,
            &labelRect,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP,
            labelColor);
    }
}

static void DrawOptionsMenu() {
    if (!spriteBrush) return;

    if (g_OptionsTitleFont) {
        RECT titleRect = { 0, OPTIONS_TITLE_TOP, SCREEN_WIDTH, OPTIONS_TITLE_BOTTOM };
        g_OptionsTitleFont->DrawTextA(
            spriteBrush,
            "OPTIONS",
            -1,
            &titleRect,
            DT_CENTER | DT_TOP,
            OPTIONS_COLOR_NORMAL);
    }

    UpdateOptionsRects();

    if (g_OptionsFont) {
        char labelBuffer[32];
        for (int i = 0; i < OPTIONS_COUNT; i++) {
            if (i == OPTIONS_BRIGHTNESS) {
                DrawBrightnessSlider(g_OptionsRects[i], i == g_OptionsSelection, g_LastHoveredOption == i);
                continue;
            }

            const D3DCOLOR textColor = (i == g_OptionsSelection)
                ? OPTIONS_COLOR_SELECTED
                : OPTIONS_COLOR_NORMAL;

            g_OptionsFont->DrawTextA(
                spriteBrush,
                GetOptionLabel(i, labelBuffer, sizeof(labelBuffer)),
                -1,
                &g_OptionsRects[i],
                DT_CENTER | DT_VCENTER | DT_SINGLELINE,
                textColor);
        }
    }

    if (g_OptionsHintFont) {
        RECT hintLine1 = {
            OPTIONS_HINT_SIDE_MARGIN,
            OPTIONS_HINT_TOP,
            SCREEN_WIDTH - OPTIONS_HINT_SIDE_MARGIN,
            OPTIONS_HINT_TOP + 20
        };
        RECT hintLine2 = {
            OPTIONS_HINT_SIDE_MARGIN,
            hintLine1.bottom,
            SCREEN_WIDTH - OPTIONS_HINT_SIDE_MARGIN,
            OPTIONS_HINT_BOTTOM
        };
        g_OptionsHintFont->DrawTextA(
            spriteBrush,
            "Up/Down / Mouse: select    Left/Right: adjust brightness",
            -1,
            &hintLine1,
            DT_CENTER | DT_TOP | DT_SINGLELINE,
            D3DCOLOR_XRGB(170, 170, 170));
        g_OptionsHintFont->DrawTextA(
            spriteBrush,
            "Enter: toggle/confirm    Esc: back",
            -1,
            &hintLine2,
            DT_CENTER | DT_TOP | DT_SINGLELINE,
            D3DCOLOR_XRGB(170, 170, 170));
    }
}

static void RenderOptionsMenu() {
    if (!spriteBrush) return;

    spriteBrush->Begin(D3DXSPRITE_ALPHABLEND);
    DrawOptionsMenu();
    spriteBrush->End();
}

static void ConfirmOptionsSelection(int& choice) {
    g_SoundManager.PlaySelectionSound();
    switch (g_OptionsSelection) {
    case OPTIONS_MUSIC:
        g_SoundManager.ToggleMusicMute();
        break;
    case OPTIONS_FPS: {
        int currentIndex = 0;
        for (int i = 0; i < kFpsChoiceCount; i++) {
            if (kFpsChoices[i] == g_TargetFPS) {
                currentIndex = i;
                break;
            }
        }
        g_TargetFPS = kFpsChoices[(currentIndex + 1) % kFpsChoiceCount];
        break;
    }
    case OPTIONS_BACK:
        choice = 2;
        break;
    }
}

static void AdjustBrightness(int delta) {
    int value = g_BrightnessLevel + delta;
    if (value < BRIGHTNESS_MIN) value = BRIGHTNESS_MIN;
    if (value > BRIGHTNESS_MAX) value = BRIGHTNESS_MAX;
    if (value != g_BrightnessLevel) {
        g_BrightnessLevel = value;
        g_SoundManager.PlaySelectionSound();
    }
}

static void UpdateOptionsInput(int& choice) {
    UpdateOptionsRects();

    bool upPressed = IsUiKeyDown(DIK_UP) || IsUiKeyDown(DIK_W);
    bool downPressed = IsUiKeyDown(DIK_DOWN) || IsUiKeyDown(DIK_S);
    bool leftPressed = IsUiKeyDown(DIK_LEFT);
    bool rightPressed = IsUiKeyDown(DIK_RIGHT);
    bool enterPressed = IsUiKeyDown(DIK_RETURN);
    bool backPressed = IsUiKeyDown(DIK_BACK) || IsUiKeyDown(DIK_ESCAPE);
    bool clickPressed = g_WindowHasFocus && ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);

    POINT cursorPt = {};
    bool hasCursor = GetGameCursorPos(cursorPt);
    int hoveredOption = -1;
    if (hasCursor) {
        for (int i = 0; i < OPTIONS_COUNT; i++) {
            if (PtInRect(&g_OptionsRects[i], cursorPt)) {
                hoveredOption = i;
                g_OptionsSelection = i;
                break;
            }
        }
    }

    if (hoveredOption != g_LastHoveredOption) {
        if (hoveredOption >= 0) {
            g_SoundManager.PlaySelectionSound();
        }
        g_LastHoveredOption = hoveredOption;
    }

    if (upPressed && !g_OptionsUpHeld) {
        g_OptionsSelection = (g_OptionsSelection - 1 + OPTIONS_COUNT) % OPTIONS_COUNT;
        g_SoundManager.PlaySelectionSound();
    }
    if (downPressed && !g_OptionsDownHeld) {
        g_OptionsSelection = (g_OptionsSelection + 1) % OPTIONS_COUNT;
        g_SoundManager.PlaySelectionSound();
    }
    g_OptionsUpHeld = upPressed;
    g_OptionsDownHeld = downPressed;

    // Left/Right act as a keyboard slider control while Brightness is selected.
    // Step once immediately on press, then repeat at a fixed rate while held.
    if (g_OptionsSelection == OPTIONS_BRIGHTNESS && (leftPressed || rightPressed)) {
        const bool freshPress = (leftPressed && !g_OptionsLeftHeld) || (rightPressed && !g_OptionsRightHeld);
        if (freshPress) {
            AdjustBrightness(leftPressed ? -BRIGHTNESS_STEP : BRIGHTNESS_STEP);
            g_BrightnessRepeatTimer = 0;
        }
        else {
            g_BrightnessRepeatTimer++;
            if (g_BrightnessRepeatTimer >= BRIGHTNESS_KEY_REPEAT_FRAMES) {
                g_BrightnessRepeatTimer = 0;
                AdjustBrightness(leftPressed ? -BRIGHTNESS_STEP : BRIGHTNESS_STEP);
            }
        }
    }
    else {
        g_BrightnessRepeatTimer = 0;
    }
    g_OptionsLeftHeld = leftPressed;
    g_OptionsRightHeld = rightPressed;

    if (enterPressed && !g_OptionsEnterHeld) {
        ConfirmOptionsSelection(choice);
    }
    g_OptionsEnterHeld = enterPressed;

    if (backPressed && !g_OptionsBackHeld) {
        g_SoundManager.PlaySelectionSound();
        choice = 2;
    }
    g_OptionsBackHeld = backPressed;

    // Start dragging the slider if the initial click lands on its track.
    if (clickPressed && !g_OptionsClickHeld && hoveredOption == OPTIONS_BRIGHTNESS && hasCursor
        && PtInRect(&g_BrightnessTrackRect, cursorPt)) {
        g_BrightnessDragging = true;
        SetBrightnessFromClientX(cursorPt.x);
        g_SoundManager.PlaySelectionSound();
    }
    else if (clickPressed && !g_OptionsClickHeld && hoveredOption >= 0) {
        ConfirmOptionsSelection(choice);
    }

    if (g_BrightnessDragging) {
        if (clickPressed && hasCursor) {
            SetBrightnessFromClientX(cursorPt.x);
        }
        else {
            g_BrightnessDragging = false;
        }
    }

    g_OptionsClickHeld = clickPressed;
}

void ResetOptionsMenuInputState() {
    g_OptionsUpHeld = IsUiKeyDown(DIK_UP) || IsUiKeyDown(DIK_W);
    g_OptionsDownHeld = IsUiKeyDown(DIK_DOWN) || IsUiKeyDown(DIK_S);
    g_OptionsLeftHeld = IsUiKeyDown(DIK_LEFT);
    g_OptionsRightHeld = IsUiKeyDown(DIK_RIGHT);
    g_OptionsEnterHeld = IsUiKeyDown(DIK_RETURN);
    g_OptionsBackHeld = IsUiKeyDown(DIK_BACK) || IsUiKeyDown(DIK_ESCAPE);
    g_OptionsClickHeld = g_WindowHasFocus && ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);
    g_OptionsSelection = 0;
    g_LastHoveredOption = -1;
    g_BrightnessDragging = false;
    g_BrightnessRepeatTimer = 0;
}

void optionsMenuScreen(int& choice) {
    if (!g_OptionsTitleFont || !g_OptionsFont || !g_OptionsHintFont) {
        LoadOptionsFont();
    }

    UpdateOptionsInput(choice);
    RenderOptionsMenu();
}
