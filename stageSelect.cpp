#include "stageSelect.h"
#include "battleBackground.h"
#include "input.h"
#include "audio.h"
#include <cstdio>

StageInfo g_Stages[] = {
    { "Stage 1", "assets/background/City3/City3.png", NULL },
    { "Stage 2", "assets/background/City4/City4.png", NULL },
    { "Stage 3", "assets/background/City2/City2.png", NULL },
};
const int STAGE_COUNT = sizeof(g_Stages) / sizeof(g_Stages[0]);

int g_SelectedStageIndex = 0;

static ID3DXFont* g_StageFont = nullptr;
static ID3DXFont* g_StageTitleFont = nullptr;
static ID3DXFont* g_StageHintFont = nullptr;
static const char* g_LoadedStageFontPath = nullptr;

static bool g_StageLeftHeld = false;
static bool g_StageRightHeld = false;
static bool g_StageEnterHeld = false;
static bool g_StageBackHeld = false;
static bool g_StageClickHeld = false;

static RECT g_StageLeftHit = {};
static RECT g_StageRightHit = {};
static RECT g_StageConfirmHit = {};

enum StageUiFocus {
    StageUi_None = -1,
    StageUi_Left = 0,
    StageUi_Confirm = 1,
    StageUi_Right = 2
};

// Yellow only while the mouse is over a control (not on enter).
static int g_StageUiFocus = StageUi_None;
static int g_LastStageUiFocus = StageUi_None;

static const D3DCOLOR STAGE_COLOR_NORMAL = D3DCOLOR_XRGB(255, 255, 255);
static const D3DCOLOR STAGE_COLOR_SELECTED = D3DCOLOR_XRGB(255, 230, 80);

// Stage select layout (named constants instead of hardcoded RECT literals).
static const float STAGE_PREVIEW_WIDTH = 640.0f;
static const float STAGE_PREVIEW_HEIGHT = 360.0f;
static const float STAGE_PREVIEW_TOP = 160.0f;
static const float STAGE_ARROW_HIT_WIDTH = 100.0f;
static const float STAGE_ARROW_HIT_HALF_HEIGHT = 48.0f;
static const float STAGE_ARROW_GAP = 10.0f;
static const float STAGE_CONFIRM_HALF_WIDTH = 220.0f;
static const LONG STAGE_TITLE_TOP = 40;
static const LONG STAGE_TITLE_BOTTOM = 100;
static const LONG STAGE_NAME_TOP = 540;
static const LONG STAGE_NAME_BOTTOM = 590;
static const LONG STAGE_COUNTER_TOP = 580;
static const LONG STAGE_COUNTER_BOTTOM = 620;
static const LONG STAGE_CONFIRM_TOP = 600;
static const LONG STAGE_CONFIRM_BOTTOM = 690;
static const LONG STAGE_HINT_TOP = 700;
static const LONG STAGE_HINT_BOTTOM = 740;
static const LONG STAGE_HINT_SIDE_MARGIN = 40;
static const float STAGE_FRAME_THICKNESS = 3.0f;

static bool CreateStageUiFont(const char* familyName, INT height, ID3DXFont** outFont) {
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

static bool LoadOrCreateStageUiFont(INT height, ID3DXFont** outFont) {
    if (*outFont) return true;

    if (!CreateStageUiFont(NORMAL_FONT_FAMILY, height, outFont)) {
        if (*outFont) {
            (*outFont)->Release();
            *outFont = nullptr;
        }
        if (!CreateStageUiFont("Arial", height, outFont)) {
            *outFont = nullptr;
            return false;
        }
    }
    return true;
}

static bool LoadStageFont() {
    if (g_StageFont && g_StageTitleFont && g_StageHintFont) {
        return true;
    }

    if (!g_LoadedStageFontPath) {
        if (AddFontResourceExA(NORMAL_FONT_FILE, FR_PRIVATE, 0) != 0) {
            g_LoadedStageFontPath = NORMAL_FONT_FILE;
        }
    }

    bool ok = true;
    if (!LoadOrCreateStageUiFont(42, &g_StageTitleFont)) ok = false;
    if (!LoadOrCreateStageUiFont(30, &g_StageFont)) ok = false;
    if (!LoadOrCreateStageUiFont(18, &g_StageHintFont)) ok = false;
    return ok;
}

bool LoadStageTextures() {
    bool allOk = true;

    for (int i = 0; i < STAGE_COUNT; i++) {
        if (g_Stages[i].texture != NULL) continue;

        HRESULT hr = D3DXCreateTextureFromFileEx(
            g_pD3DDevice,
            g_Stages[i].texturePath,
            D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT,
            NULL, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT,
            0, NULL, NULL, &g_Stages[i].texture);

        if (FAILED(hr)) {
            g_Stages[i].texture = NULL;
            allOk = false;
        }
    }

    if (!LoadStageFont()) {
        allOk = false;
    }

    return allOk;
}

void CleanUpStageTextures() {
    CleanUpBattleParallax();

    for (int i = 0; i < STAGE_COUNT; i++) {
        if (g_Stages[i].texture) {
            if (texBgCity1 == g_Stages[i].texture) {
                texBgCity1 = NULL;
            }
            g_Stages[i].texture->Release();
            g_Stages[i].texture = NULL;
        }
    }

    if (g_StageFont) {
        g_StageFont->Release();
        g_StageFont = nullptr;
    }
    if (g_StageTitleFont) {
        g_StageTitleFont->Release();
        g_StageTitleFont = nullptr;
    }
    if (g_StageHintFont) {
        g_StageHintFont->Release();
        g_StageHintFont = nullptr;
    }
}

void NotifyStageDeviceLost() {
    if (g_StageFont) g_StageFont->OnLostDevice();
    if (g_StageTitleFont) g_StageTitleFont->OnLostDevice();
    if (g_StageHintFont) g_StageHintFont->OnLostDevice();
}

void NotifyStageDeviceReset() {
    if (g_StageFont && FAILED(g_StageFont->OnResetDevice())) {
        g_StageFont->Release();
        g_StageFont = nullptr;
    }
    if (g_StageTitleFont && FAILED(g_StageTitleFont->OnResetDevice())) {
        g_StageTitleFont->Release();
        g_StageTitleFont = nullptr;
    }
    if (g_StageHintFont && FAILED(g_StageHintFont->OnResetDevice())) {
        g_StageHintFont->Release();
        g_StageHintFont = nullptr;
    }
    LoadStageFont();
}

void ApplySelectedStageToBattle() {
    if (g_SelectedStageIndex >= STAGE_COUNT) {
        g_SelectedStageIndex = 0;
    }
    if (g_SelectedStageIndex >= 0 && g_SelectedStageIndex < STAGE_COUNT) {
        texBgCity1 = g_Stages[g_SelectedStageIndex].texture;
        if (!LoadBattleParallaxForStage(g_SelectedStageIndex)) {
            // Fall back to the composite preview texture if layer load fails.
        }
    }
}

static void GetStagePreviewLayout(float& previewW, float& previewH, float& offsetX, float& offsetY) {
    previewW = STAGE_PREVIEW_WIDTH;
    previewH = STAGE_PREVIEW_HEIGHT;
    offsetX = ((float)SCREEN_WIDTH - previewW) * 0.5f;
    offsetY = STAGE_PREVIEW_TOP;
}

static void UpdateStageUiHitboxes() {
    float previewW = 0.0f;
    float previewH = 0.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    GetStagePreviewLayout(previewW, previewH, offsetX, offsetY);

    const float arrowCenterY = offsetY + previewH * 0.5f;
    g_StageLeftHit = {
        (LONG)(offsetX - STAGE_ARROW_GAP - STAGE_ARROW_HIT_WIDTH),
        (LONG)(arrowCenterY - STAGE_ARROW_HIT_HALF_HEIGHT),
        (LONG)(offsetX - STAGE_ARROW_GAP),
        (LONG)(arrowCenterY + STAGE_ARROW_HIT_HALF_HEIGHT)
    };
    g_StageRightHit = {
        (LONG)(offsetX + previewW + STAGE_ARROW_GAP),
        (LONG)(arrowCenterY - STAGE_ARROW_HIT_HALF_HEIGHT),
        (LONG)(offsetX + previewW + STAGE_ARROW_GAP + STAGE_ARROW_HIT_WIDTH),
        (LONG)(arrowCenterY + STAGE_ARROW_HIT_HALF_HEIGHT)
    };

    // Wide / tall hitbox so Confirm is easy to click (text stays centered inside).
    const float previewCenterX = offsetX + previewW * 0.5f;
    g_StageConfirmHit = {
        (LONG)(previewCenterX - STAGE_CONFIRM_HALF_WIDTH),
        STAGE_CONFIRM_TOP,
        (LONG)(previewCenterX + STAGE_CONFIRM_HALF_WIDTH),
        STAGE_CONFIRM_BOTTOM
    };
}

static void DrawStagePreview() {
    if (!spriteBrush) return;

    const StageInfo& stage = g_Stages[g_SelectedStageIndex];

    float previewW = 0.0f;
    float previewH = 0.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    GetStagePreviewLayout(previewW, previewH, offsetX, offsetY);

    if (g_StageTitleFont) {
        RECT titleRect = { 0, STAGE_TITLE_TOP, SCREEN_WIDTH, STAGE_TITLE_BOTTOM };
        g_StageTitleFont->DrawTextA(
            spriteBrush,
            "STAGE SELECT",
            -1,
            &titleRect,
            DT_CENTER | DT_TOP,
            STAGE_COLOR_NORMAL);
    }

    if (stage.texture) {
        D3DSURFACE_DESC desc;
        stage.texture->GetLevelDesc(0, &desc);
        float scaleX = previewW / (float)desc.Width;
        float scaleY = previewH / (float)desc.Height;

        D3DXMATRIX matScale, matTrans, matFinal;
        D3DXMatrixScaling(&matScale, scaleX, scaleY, 1.0f);
        D3DXMatrixTranslation(&matTrans, offsetX, offsetY, 0.0f);
        matFinal = matScale * matTrans;
        spriteBrush->SetTransform(&matFinal);

        D3DXVECTOR3 zeroPos(0.0f, 0.0f, 0.0f);
        spriteBrush->Draw(stage.texture, NULL, NULL, &zeroPos, D3DCOLOR_XRGB(255, 255, 255));

        D3DXMATRIX matIdentity;
        D3DXMatrixIdentity(&matIdentity);
        spriteBrush->SetTransform(&matIdentity);
    }
    else {
        DrawDebugRect(spriteBrush, offsetX, offsetY, previewW, previewH, D3DCOLOR_ARGB(255, 40, 40, 40));
    }

    const float frame = STAGE_FRAME_THICKNESS;
    DrawDebugRect(spriteBrush, offsetX - frame, offsetY - frame, previewW + frame * 2.0f, frame, D3DCOLOR_ARGB(220, 255, 255, 255));
    DrawDebugRect(spriteBrush, offsetX - frame, offsetY + previewH, previewW + frame * 2.0f, frame, D3DCOLOR_ARGB(220, 255, 255, 255));
    DrawDebugRect(spriteBrush, offsetX - frame, offsetY - frame, frame, previewH + frame * 2.0f, D3DCOLOR_ARGB(220, 255, 255, 255));
    DrawDebugRect(spriteBrush, offsetX + previewW, offsetY - frame, frame, previewH + frame * 2.0f, D3DCOLOR_ARGB(220, 255, 255, 255));

    UpdateStageUiHitboxes();

    if (g_StageFont) {
        const D3DCOLOR leftColor = (g_StageUiFocus == StageUi_Left) ? STAGE_COLOR_SELECTED : STAGE_COLOR_NORMAL;
        const D3DCOLOR rightColor = (g_StageUiFocus == StageUi_Right) ? STAGE_COLOR_SELECTED : STAGE_COLOR_NORMAL;
        const D3DCOLOR confirmColor = (g_StageUiFocus == StageUi_Confirm) ? STAGE_COLOR_SELECTED : STAGE_COLOR_NORMAL;

        g_StageFont->DrawTextA(spriteBrush, "<", -1, &g_StageLeftHit, DT_CENTER | DT_VCENTER | DT_SINGLELINE, leftColor);
        g_StageFont->DrawTextA(spriteBrush, ">", -1, &g_StageRightHit, DT_CENTER | DT_VCENTER | DT_SINGLELINE, rightColor);

        RECT nameRect = { 0, STAGE_NAME_TOP, SCREEN_WIDTH, STAGE_NAME_BOTTOM };
        g_StageFont->DrawTextA(spriteBrush, stage.name, -1, &nameRect, DT_CENTER | DT_TOP, STAGE_COLOR_NORMAL);

        char counterText[32];
        sprintf_s(counterText, "%d / %d", g_SelectedStageIndex + 1, STAGE_COUNT);
        RECT counterRect = { 0, STAGE_COUNTER_TOP, SCREEN_WIDTH, STAGE_COUNTER_BOTTOM };
        g_StageFont->DrawTextA(spriteBrush, counterText, -1, &counterRect, DT_CENTER | DT_TOP, D3DCOLOR_XRGB(220, 220, 220));

        g_StageFont->DrawTextA(
            spriteBrush,
            "Confirm",
            -1,
            &g_StageConfirmHit,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE,
            confirmColor);

        if (g_StageHintFont) {
            RECT hintRect = {
                STAGE_HINT_SIDE_MARGIN,
                STAGE_HINT_TOP,
                SCREEN_WIDTH - STAGE_HINT_SIDE_MARGIN,
                STAGE_HINT_BOTTOM
            };
            g_StageHintFont->DrawTextA(
                spriteBrush,
                "Arrows / Mouse: change stage    Enter: Confirm    Esc: back",
                -1,
                &hintRect,
                DT_CENTER | DT_TOP | DT_SINGLELINE,
                D3DCOLOR_XRGB(170, 170, 170));
        }
    }
}

static void RenderStageSelect() {
    if (!spriteBrush) return;

    spriteBrush->Begin(D3DXSPRITE_ALPHABLEND);
    DrawStagePreview();
    spriteBrush->End();
}

static void UpdateStageUiFocusFromCursor() {
    POINT cursorPt = {};
    if (!GetGameCursorPos(cursorPt)) {
        g_StageUiFocus = StageUi_None;
    }
    else if (PtInRect(&g_StageLeftHit, cursorPt)) {
        g_StageUiFocus = StageUi_Left;
    }
    else if (PtInRect(&g_StageRightHit, cursorPt)) {
        g_StageUiFocus = StageUi_Right;
    }
    else if (PtInRect(&g_StageConfirmHit, cursorPt)) {
        g_StageUiFocus = StageUi_Confirm;
    }
    else {
        g_StageUiFocus = StageUi_None;
    }

    if (g_StageUiFocus != g_LastStageUiFocus) {
        if (g_StageUiFocus != StageUi_None) {
            g_SoundManager.PlaySelectionSound();
        }
        g_LastStageUiFocus = g_StageUiFocus;
    }
}

static void UpdateStageSelectInput(int& choice) {
    UpdateStageUiHitboxes();
    UpdateStageUiFocusFromCursor();

    bool leftPressed = (diKeys[DIK_LEFT] & 0x80) != 0 || (diKeys[DIK_A] & 0x80) != 0;
    bool rightPressed = (diKeys[DIK_RIGHT] & 0x80) != 0 || (diKeys[DIK_D] & 0x80) != 0;
    bool enterPressed = (diKeys[DIK_RETURN] & 0x80) != 0;
    bool backPressed = (diKeys[DIK_BACK] & 0x80) != 0 || (diKeys[DIK_ESCAPE] & 0x80) != 0;
    bool clickPressed = g_WindowHasFocus && ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);

    POINT cursorPt = {};
    bool hasCursor = GetGameCursorPos(cursorPt);

    if (leftPressed && !g_StageLeftHeld) {
        g_SelectedStageIndex = (g_SelectedStageIndex - 1 + STAGE_COUNT) % STAGE_COUNT;
        g_SoundManager.PlaySelectionSound();
    }
    if (rightPressed && !g_StageRightHeld) {
        g_SelectedStageIndex = (g_SelectedStageIndex + 1) % STAGE_COUNT;
        g_SoundManager.PlaySelectionSound();
    }
    g_StageLeftHeld = leftPressed;
    g_StageRightHeld = rightPressed;

    if (enterPressed && !g_StageEnterHeld) {
        g_SoundManager.PlaySelectionSound();
        choice = 1;
    }
    g_StageEnterHeld = enterPressed;

    if (backPressed && !g_StageBackHeld) {
        g_SoundManager.PlaySelectionSound();
        choice = 2;
    }
    g_StageBackHeld = backPressed;

    if (clickPressed && !g_StageClickHeld && hasCursor) {
        if (PtInRect(&g_StageLeftHit, cursorPt)) {
            g_StageUiFocus = StageUi_Left;
            g_SelectedStageIndex = (g_SelectedStageIndex - 1 + STAGE_COUNT) % STAGE_COUNT;
            g_SoundManager.PlaySelectionSound();
        }
        else if (PtInRect(&g_StageRightHit, cursorPt)) {
            g_StageUiFocus = StageUi_Right;
            g_SelectedStageIndex = (g_SelectedStageIndex + 1) % STAGE_COUNT;
            g_SoundManager.PlaySelectionSound();
        }
        else if (PtInRect(&g_StageConfirmHit, cursorPt)) {
            g_StageUiFocus = StageUi_Confirm;
            g_SoundManager.PlaySelectionSound();
            choice = 1;
        }
    }
    g_StageClickHeld = clickPressed;
}

void ResetStageSelectInputState() {
    g_StageLeftHeld = (diKeys[DIK_LEFT] & 0x80) != 0 || (diKeys[DIK_A] & 0x80) != 0;
    g_StageRightHeld = (diKeys[DIK_RIGHT] & 0x80) != 0 || (diKeys[DIK_D] & 0x80) != 0;
    g_StageEnterHeld = (diKeys[DIK_RETURN] & 0x80) != 0;
    g_StageBackHeld = (diKeys[DIK_BACK] & 0x80) != 0 || (diKeys[DIK_ESCAPE] & 0x80) != 0;
    g_StageClickHeld = g_WindowHasFocus && ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);
    g_StageUiFocus = StageUi_None;
    g_LastStageUiFocus = StageUi_None;
}

void stageSelectScreen(int& choice) {
    if (!g_StageFont || !g_StageTitleFont || !g_StageHintFont) {
        LoadStageFont();
    }

    UpdateStageSelectInput(choice);
    RenderStageSelect();
}
