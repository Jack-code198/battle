#include "menuMain.h"
#include "input.h"

static LPDIRECT3DTEXTURE9 menuBackground = NULL;
static ID3DXFont* menuFont = nullptr;
static ID3DXFont* titleFont = nullptr;
static const char* g_LoadedMenuFontPath = nullptr;

static int currentSelection = 0;

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

static bool LoadMenuFont() {
    if (menuFont && titleFont) {
        return true;
    }

    if (!g_LoadedMenuFontPath) {
        const char* fontCandidates[] = { HUD_FONT_FILE, HUD_FONT_FILE_ALT };
        for (const char* path : fontCandidates) {
            if (AddFontResourceExA(path, FR_PRIVATE, 0) != 0) {
                g_LoadedMenuFontPath = path;
                break;
            }
        }
    }

    auto tryCreateFont = [](const char* familyName, INT height, ID3DXFont** outFont) -> HRESULT {
        return D3DXCreateFontA(
            g_pD3DDevice,
            height,
            0,
            FW_BOLD,
            1,
            FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            familyName,
            outFont);
    };

    bool ok = true;

    if (!titleFont) {
        HRESULT hr = tryCreateFont(HUD_FONT_FAMILY, 52, &titleFont);
        if (FAILED(hr) || !titleFont) {
            if (titleFont) {
                titleFont->Release();
                titleFont = nullptr;
            }
            hr = tryCreateFont("Arial", 52, &titleFont);
            if (FAILED(hr) || !titleFont) {
                titleFont = nullptr;
                ok = false;
            }
        }
    }

    if (!menuFont) {
        HRESULT hr = tryCreateFont(HUD_FONT_FAMILY, 34, &menuFont);
        if (FAILED(hr) || !menuFont) {
            if (menuFont) {
                menuFont->Release();
                menuFont = nullptr;
            }
            hr = tryCreateFont("Arial", 34, &menuFont);
            if (FAILED(hr) || !menuFont) {
                menuFont = nullptr;
                ok = false;
            }
        }
    }

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

    RECT personaRect = {
        left,
        MENU_TITLE_TOP,
        right,
        MENU_TITLE_TOP + MENU_TITLE_LINE_HEIGHT
    };
    titleFont->DrawTextA(
        spriteBrush,
        "PERSONA",
        -1,
        &personaRect,
        DT_RIGHT | DT_VCENTER | DT_SINGLELINE,
        titleColor);

    RECT arenaRect = {
        left,
        MENU_TITLE_TOP + MENU_TITLE_LINE_HEIGHT,
        right,
        MENU_TITLE_TOP + (MENU_TITLE_LINE_HEIGHT * 2)
    };
    titleFont->DrawTextA(
        spriteBrush,
        "ARENA",
        -1,
        &arenaRect,
        DT_RIGHT | DT_VCENTER | DT_SINGLELINE,
        titleColor);
}

void drawMenuOptions() {
    if (!menuFont) return;

    UpdateOptionRects();

    for (int i = 0; i < MENU_OPTION_COUNT; i++) {
        const bool selected = (i == currentSelection);
        const D3DCOLOR textColor = selected
            ? D3DCOLOR_XRGB(255, 230, 80)
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

    if (upPressed && !g_MenuUpHeld) {
        currentSelection = (currentSelection - 1 + MENU_OPTION_COUNT) % MENU_OPTION_COUNT;
    }
    if (downPressed && !g_MenuDownHeld) {
        currentSelection = (currentSelection + 1) % MENU_OPTION_COUNT;
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
}

void mainMenu(int& choice) {
    if (!menuBackground || !menuFont || !titleFont) {
        LoadMenuTextures();
    }

    UpdateMenuInput(choice);
    renderMainMenu();
}
