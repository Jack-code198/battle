#include "stageSelect.h"
#include <cstdio>

// Stage list
StageInfo g_Stages[] = {
    { "Stage 1",   "assets/background/City3/city3.png",   NULL },
    { "Stage 2","assets/background/City4/City4.png", NULL },
    { "Stage 3","assets/background/City2/City2.png", NULL },
    { "Stage 4","assets/background/City1/City1.png", NULL },
};
const int STAGE_COUNT = sizeof(g_Stages) / sizeof(g_Stages[0]);

int g_SelectedStageIndex = 0;

static ID3DXFont* g_StageFont = nullptr;

// Edge-detection state so holding a key doesn't spam-cycle stages
static bool g_StageLeftHeld = false;
static bool g_StageRightHeld = false;
static bool g_StageEnterHeld = false;
static bool g_StageBackHeld = false;

// Loading and cleanup 
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
            allOk = false; // missing/placeholder asset - preview will show a fallback box
        }
    }

    if (g_StageFont == nullptr) {
        D3DXCreateFontA(
            g_pD3DDevice,
            32, 0, FW_BOLD, 1, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            "Arial",
            &g_StageFont);
    }

    return allOk;
}

void CleanUpStageTextures() {
    for (int i = 0; i < STAGE_COUNT; i++) {
        if (g_Stages[i].texture) {
            // texBgCity1 may currently be aliasing this exact texture
            // (see ApplySelectedStageToBattle) - clear that alias first so
            // nothing downstream (e.g. renderer.cpp's CleanUpD3D) tries to
            // release it a second time.
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
}

void ApplySelectedStageToBattle() {
    if (g_SelectedStageIndex >= 0 && g_SelectedStageIndex < STAGE_COUNT) {
        texBgCity1 = g_Stages[g_SelectedStageIndex].texture;
    }
}

// Drawing
static void DrawStagePreview() {
    if (!spriteBrush) return;

    const StageInfo& stage = g_Stages[g_SelectedStageIndex];

    const float previewW = 640.0f;
    const float previewH = 360.0f;
    const float offsetX = ((float)SCREEN_WIDTH - previewW) * 0.5f;
    const float offsetY = 160.0f;

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
        // Texture failed to load (e.g. placeholder path) - show a visible
        // fallback box instead of silently drawing nothing.
        DrawDebugRect(spriteBrush, offsetX, offsetY, previewW, previewH, D3DCOLOR_ARGB(255, 40, 40, 40));
    }

    DrawDebugRect(spriteBrush, offsetX - 3.0f, offsetY - 3.0f, previewW + 6.0f, 3.0f, D3DCOLOR_ARGB(220, 255, 255, 255));
    DrawDebugRect(spriteBrush, offsetX - 3.0f, offsetY + previewH, previewW + 6.0f, 3.0f, D3DCOLOR_ARGB(220, 255, 255, 255));
    DrawDebugRect(spriteBrush, offsetX - 3.0f, offsetY - 3.0f, 3.0f, previewH + 6.0f, D3DCOLOR_ARGB(220, 255, 255, 255));
    DrawDebugRect(spriteBrush, offsetX + previewW, offsetY - 3.0f, 3.0f, previewH + 6.0f, D3DCOLOR_ARGB(220, 255, 255, 255));

    if (g_StageFont) {
        RECT nameRect = { 0, 540, SCREEN_WIDTH, 590 };
        g_StageFont->DrawTextA(spriteBrush, stage.name, -1, &nameRect, DT_CENTER | DT_TOP, D3DCOLOR_XRGB(255, 255, 0));

        char hintText[96];
        sprintf_s(hintText, "%d / %d    <-  ->  choose stage    Enter confirm    Esc back",
            g_SelectedStageIndex + 1, STAGE_COUNT);
        RECT hintRect = { 0, 600, SCREEN_WIDTH, 640 };
        g_StageFont->DrawTextA(spriteBrush, hintText, -1, &hintRect, DT_CENTER | DT_TOP, D3DCOLOR_XRGB(200, 200, 200));
    }
}

static void RenderStageSelect() {
    if (!spriteBrush) return;

    spriteBrush->Begin(D3DXSPRITE_ALPHABLEND);
    DrawStagePreview();
    spriteBrush->End();
}

// Input
static void UpdateStageSelectInput(int& choice) {
    bool leftPressed = (diKeys[DIK_LEFT] & 0x80) != 0 || (diKeys[DIK_A] & 0x80) != 0;
    bool rightPressed = (diKeys[DIK_RIGHT] & 0x80) != 0 || (diKeys[DIK_D] & 0x80) != 0;
    bool enterPressed = (diKeys[DIK_RETURN] & 0x80) != 0;
    bool backPressed = (diKeys[DIK_BACK] & 0x80) != 0 || (diKeys[DIK_ESCAPE] & 0x80) != 0;

    if (leftPressed && !g_StageLeftHeld) {
        g_SelectedStageIndex = (g_SelectedStageIndex - 1 + STAGE_COUNT) % STAGE_COUNT;
    }
    if (rightPressed && !g_StageRightHeld) {
        g_SelectedStageIndex = (g_SelectedStageIndex + 1) % STAGE_COUNT;
    }
    g_StageLeftHeld = leftPressed;
    g_StageRightHeld = rightPressed;

    if (enterPressed && !g_StageEnterHeld) {
        choice = 1;
    }
    g_StageEnterHeld = enterPressed;

    if (backPressed && !g_StageBackHeld) {
        choice = 2;
    }
    g_StageBackHeld = backPressed;
}
void ResetStageSelectInputState() {
    g_StageLeftHeld = (diKeys[DIK_LEFT] & 0x80) != 0 || (diKeys[DIK_A] & 0x80) != 0;
    g_StageRightHeld = (diKeys[DIK_RIGHT] & 0x80) != 0 || (diKeys[DIK_D] & 0x80) != 0;
    g_StageEnterHeld = (diKeys[DIK_RETURN] & 0x80) != 0;
    g_StageBackHeld = (diKeys[DIK_BACK] & 0x80) != 0 || (diKeys[DIK_ESCAPE] & 0x80) != 0;
}
void stageSelectScreen(int& choice) {
    UpdateStageSelectInput(choice);
    RenderStageSelect();
}