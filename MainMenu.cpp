#include "MainMenu.h"
#include "input.h"
#include "audio.h"

static LPDIRECT3DTEXTURE9 menuBackground = NULL;
static ID3DXFont* menuFont = nullptr;
static ID3DXFont* titleFont = nullptr;
static bool g_MenuFontsRegistered = false;

static int currentSelection = 0;
static int g_LastHoveredOption = -1;

static bool g_MenuUpHeld = false;
static bool g_MenuDownHeld = false;
static bool g_MenuEnterHeld = false;
static bool g_MenuClickHeld = false;

static const int MENU_OPTION_COUNT = 3;
static const char* g_MenuOptions[MENU_OPTION_COUNT] = {
    "Start Game",
    "Options",
    "Exit"
};

static RECT g_OptionRects[MENU_OPTION_COUNT] = {};

static const int MENU_OPTION_WIDTH = 320;
static const int MENU_OPTION_HEIGHT = 44;
static const int MENU_OPTION_SPACING = 52;
static const int MENU_RIGHT_MARGIN = 48;
static const int MENU_TITLE_WIDTH = 420;
static const int MENU_TITLE_TOP = 40;
static const int MENU_TITLE_LINE_HEIGHT = 56;
static const int MENU_TOP_MARGIN = MENU_TITLE_TOP + (MENU_TITLE_LINE_HEIGHT * 2) + 20;

static HRESULT CreateUiFont(const char* familyName, INT height, BOOL italic, ID3DXFont** outFont) {
    // ClearType on a saturated red background looks muddy; antialias stays sharper.
    return D3DXCreateFontA(
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
}

static bool LoadOrCreateUiFont(const char* familyName, INT height, BOOL italic, ID3DXFont** outFont) {
    if (*outFont) return true;

    HRESULT hr = CreateUiFont(familyName, height, italic, outFont);
    if (FAILED(hr) || !*outFont) {
        if (*outFont) {
            (*outFont)->Release();
            *outFont = nullptr;
        }
        hr = CreateUiFont("Arial", height, italic, outFont);
        if (FAILED(hr) || !*outFont) {
            *outFont = nullptr;
            return false;
        }
    }
    return true;
}

static bool LoadMenuFont() {
    if (menuFont && titleFont) {
        return true;
    }

    if (!g_MenuFontsRegistered) {
        AddFontResourceExA(GAMETITLE_FONT_FILE, FR_PRIVATE, 0);
        AddFontResourceExA(MAINMENU_FONT_FILE, FR_PRIVATE, 0);
        g_MenuFontsRegistered = true;
    }

    bool ok = true;
    // Title stays upright/aligned; italic only leans letters to the right.
    if (!LoadOrCreateUiFont(GAMETITLE_FONT_FAMILY, 56, TRUE, &titleFont)) ok = false;
    if (!LoadOrCreateUiFont(MAINMENU_FONT_FAMILY, 34, FALSE, &menuFont)) ok = false;
    return ok;
}

bool LoadMenuTextures() {
    bool ok = true;

    if (menuBackground == NULL) {
        // File on disk is MainMenu.jpg (user-facing name may say .png).
        HRESULT hr = D3DXCreateTextureFromFileEx(
            g_pD3DDevice,
            "assets/background/MainMenu.jpg",
            D3DX_DEFAULT_NONPOW2,
            D3DX_DEFAULT_NONPOW2,
            D3DX_DEFAULT,
            NULL,
            D3DFMT_A8R8G8B8,
            D3DPOOL_MANAGED,
            D3DX_DEFAULT,
            D3DX_DEFAULT,
            0,
            NULL,
            NULL,
            &menuBackground);

        if (FAILED(hr)) {
            menuBackground = NULL;
            ok = false;
        }
    }

    if (!LoadMenuFont()) {
        ok = false;
    }

    return ok;
}

void CleanUpMenuTextures() {
    if (menuFont) {
        menuFont->Release();
        menuFont = nullptr;
    }
    if (titleFont) {
        titleFont->Release();
        titleFont = nullptr;
    }
    if (menuBackground) {
        menuBackground->Release();
        menuBackground = NULL;
    }
}

void NotifyMenuDeviceLost() {
    if (menuFont) menuFont->OnLostDevice();
    if (titleFont) titleFont->OnLostDevice();
}

void NotifyMenuDeviceReset() {
    if (menuFont && FAILED(menuFont->OnResetDevice())) {
        menuFont->Release();
        menuFont = nullptr;
    }
    if (titleFont && FAILED(titleFont->OnResetDevice())) {
        titleFont->Release();
        titleFont = nullptr;
    }
    LoadMenuFont();
}

static void UpdateOptionRects() {
    const int startX = SCREEN_WIDTH - MENU_RIGHT_MARGIN - MENU_OPTION_WIDTH;
    const int startY = MENU_TOP_MARGIN;

    for (int i = 0; i < MENU_OPTION_COUNT; i++) {
        g_OptionRects[i].left = startX;
        g_OptionRects[i].top = startY + (i * MENU_OPTION_SPACING);
        g_OptionRects[i].right = startX + MENU_OPTION_WIDTH;
        g_OptionRects[i].bottom = g_OptionRects[i].top + MENU_OPTION_HEIGHT;
    }
}

static void DrawFullscreenBackground() {
    if (!menuBackground || !spriteBrush) return;

    D3DSURFACE_DESC desc;
    menuBackground->GetLevelDesc(0, &desc);
    if (desc.Width == 0 || desc.Height == 0) return;

    const float scaleX = (float)SCREEN_WIDTH / (float)desc.Width;
    const float scaleY = (float)SCREEN_HEIGHT / (float)desc.Height;

    D3DXMATRIX matScale, matTrans, matFinal;
    D3DXMatrixScaling(&matScale, scaleX, scaleY, 1.0f);
    D3DXMatrixTranslation(&matTrans, 0.0f, 0.0f, 0.0f);
    matFinal = matScale * matTrans;
    spriteBrush->SetTransform(&matFinal);

    D3DXVECTOR3 zeroPos(0.0f, 0.0f, 0.0f);
    spriteBrush->Draw(menuBackground, NULL, NULL, &zeroPos, D3DCOLOR_XRGB(255, 255, 255));

    D3DXMATRIX matIdentity;
    D3DXMatrixIdentity(&matIdentity);
    spriteBrush->SetTransform(&matIdentity);
}

static void DrawTitle() {
    if (!titleFont) return;

    const int right = SCREEN_WIDTH - MENU_RIGHT_MARGIN;
    const int left = right - MENU_TITLE_WIDTH;
    const D3DCOLOR titleColor = D3DCOLOR_XRGB(255, 255, 255);
    const DWORD format = DT_RIGHT | DT_VCENTER | DT_SINGLELINE;

    RECT personaRect = {
        left,
        MENU_TITLE_TOP,
        right,
        MENU_TITLE_TOP + MENU_TITLE_LINE_HEIGHT
    };
    titleFont->DrawTextA(spriteBrush, "PERSONA", -1, &personaRect, format, titleColor);

    RECT arenaRect = {
        left,
        MENU_TITLE_TOP + MENU_TITLE_LINE_HEIGHT,
        right,
        MENU_TITLE_TOP + (MENU_TITLE_LINE_HEIGHT * 2)
    };
    titleFont->DrawTextA(spriteBrush, "ARENA", -1, &arenaRect, format, titleColor);
}

void drawMenuOptions() {
    if (!menuFont) return;

    UpdateOptionRects();

    for (int i = 0; i < MENU_OPTION_COUNT; i++) {
        // Same as old yellow highlight: selected text color changes (now black).
        const D3DCOLOR textColor = (i == currentSelection)
            ? D3DCOLOR_XRGB(0, 0, 0)
            : D3DCOLOR_XRGB(255, 255, 255);

        menuFont->DrawTextA(
            spriteBrush,
            g_MenuOptions[i],
            -1,
            &g_OptionRects[i],
            DT_RIGHT | DT_VCENTER | DT_SINGLELINE,
            textColor);
    }
}

void renderMainMenu() {
    if (!spriteBrush) return;

    spriteBrush->Begin(D3DXSPRITE_ALPHABLEND);
    DrawFullscreenBackground();
    DrawTitle();
    drawMenuOptions();
    spriteBrush->End();
}

static void ConfirmMenuSelection(int& choice) {
    g_SoundManager.PlaySelectionSound();
    switch (currentSelection) {
    case 0:
        choice = 1;
        break;
    case 1:
        choice = 2;
        break;
    case 2:
        PostQuitMessage(0);
        break;
    }
}

static void UpdateMenuInput(int& choice) {
    UpdateOptionRects();

    bool upPressed = (diKeys[DIK_UP] & 0x80) != 0 || (diKeys[DIK_W] & 0x80) != 0;
    bool downPressed = (diKeys[DIK_DOWN] & 0x80) != 0 || (diKeys[DIK_S] & 0x80) != 0;
    bool enterPressed = (diKeys[DIK_RETURN] & 0x80) != 0;
    bool clickPressed = g_WindowHasFocus && ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);

    POINT cursorPt = {};
    bool hasCursor = GetGameCursorPos(cursorPt);
    int hoveredOption = -1;
    if (hasCursor) {
        for (int i = 0; i < MENU_OPTION_COUNT; i++) {
            if (PtInRect(&g_OptionRects[i], cursorPt)) {
                hoveredOption = i;
                currentSelection = i;
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

    if (upPressed && !g_MenuUpHeld) {
        currentSelection = (currentSelection - 1 + MENU_OPTION_COUNT) % MENU_OPTION_COUNT;
        g_SoundManager.PlaySelectionSound();
    }
    if (downPressed && !g_MenuDownHeld) {
        currentSelection = (currentSelection + 1) % MENU_OPTION_COUNT;
        g_SoundManager.PlaySelectionSound();
    }

    g_MenuUpHeld = upPressed;
    g_MenuDownHeld = downPressed;

    if (enterPressed && !g_MenuEnterHeld) {
        ConfirmMenuSelection(choice);
    }
    g_MenuEnterHeld = enterPressed;

    if (clickPressed && !g_MenuClickHeld && hoveredOption >= 0) {
        currentSelection = hoveredOption;
        ConfirmMenuSelection(choice);
    }
    g_MenuClickHeld = clickPressed;
}

void ResetMenuInputState() {
    g_MenuUpHeld = (diKeys[DIK_UP] & 0x80) != 0 || (diKeys[DIK_W] & 0x80) != 0;
    g_MenuDownHeld = (diKeys[DIK_DOWN] & 0x80) != 0 || (diKeys[DIK_S] & 0x80) != 0;
    g_MenuEnterHeld = (diKeys[DIK_RETURN] & 0x80) != 0;
    g_MenuClickHeld = g_WindowHasFocus && ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);
    g_LastHoveredOption = -1;
}

void mainMenu(int& choice) {
    if (!menuBackground || !menuFont || !titleFont) {
        LoadMenuTextures();
    }

    UpdateMenuInput(choice);
    renderMainMenu();
}
