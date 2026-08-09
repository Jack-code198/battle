#include "menuMain.h"

// Variables
static LPDIRECT3DTEXTURE9 menuBackground = NULL;
static ID3DXFont* menuFont = nullptr;
static D3DXVECTOR3 backgroundPosition = D3DXVECTOR3(0, 0, 0);

static int currentSelection = 0;

// Edge-detection state so holding a key doesn't spam the selection change
static bool g_MenuUpHeld = false;
static bool g_MenuDownHeld = false;
static bool g_MenuEnterHeld = false;

static const int MENU_OPTION_COUNT = 3;
static const char* g_MenuOptions[MENU_OPTION_COUNT] = {
    "Start Game",
    "Options",
    "Exit"
};

// Loading and cleanup 
bool LoadMenuTextures() {
    bool ok = true;

    if (menuBackground == NULL) {
        HRESULT hr = D3DXCreateTextureFromFileEx(g_pD3DDevice, "assets/background/menu_background.png", D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT, NULL, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &menuBackground);

        if (FAILED(hr)) {
            menuBackground = NULL;
            ok = false;
        }
    }

    if (menuFont == nullptr) {
        HRESULT hr = D3DXCreateFontA(g_pD3DDevice, 36, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,"Arial",&menuFont);

        if (FAILED(hr) || !menuFont) {
            menuFont = nullptr;
            ok = false;
        }
    }

    return ok;
}

void CleanUpMenuTextures() {
    if (menuFont) {
        menuFont->Release();
        menuFont = nullptr;
    }
    if (menuBackground) {
        menuBackground->Release();
        menuBackground = NULL;
    }
}

// Drawing
void drawMenuOptions() {
    if (!menuFont) return;

    int startX = 500;
    int startY = 300;
    int spacing = 60;

    for (int i = 0; i < MENU_OPTION_COUNT; i++) {
        RECT textRect;
        textRect.left = startX;
        textRect.top = startY + (i * spacing);
        textRect.right = startX + 300;
        textRect.bottom = textRect.top + 50;

        // Highlight the currently selected option
        D3DCOLOR textColor = (i == currentSelection)
            ? D3DCOLOR_XRGB(255, 255, 0)   // Yellow
            : D3DCOLOR_XRGB(255, 255, 255); // White

        menuFont->DrawTextA(spriteBrush, g_MenuOptions[i], -1, &textRect, DT_LEFT | DT_TOP, textColor);
    }
}

// For displaying the main menu
void renderMainMenu() {
    if (!spriteBrush) return;

    // Begin sprite drawing
    spriteBrush->Begin(D3DXSPRITE_ALPHABLEND);

    // Draw the background
    if (menuBackground != NULL) {
        spriteBrush->Draw(menuBackground, NULL, NULL, &backgroundPosition, D3DCOLOR_XRGB(255, 255, 255));
    }

    // Draw menu options
    drawMenuOptions();

    // End sprite drawing
    spriteBrush->End();
}

// Input
static void UpdateMenuInput(int& choice) {
    bool upPressed = (diKeys[DIK_UP] & 0x80) != 0 || (diKeys[DIK_W] & 0x80) != 0;
    bool downPressed = (diKeys[DIK_DOWN] & 0x80) != 0 || (diKeys[DIK_S] & 0x80) != 0;
    bool enterPressed = (diKeys[DIK_RETURN] & 0x80) != 0;

    if (upPressed && !g_MenuUpHeld) {
        currentSelection = (currentSelection - 1 + MENU_OPTION_COUNT) % MENU_OPTION_COUNT;
    }
    if (downPressed && !g_MenuDownHeld) {
        currentSelection = (currentSelection + 1) % MENU_OPTION_COUNT;
    }

    g_MenuUpHeld = upPressed;
    g_MenuDownHeld = downPressed;

    if (enterPressed && !g_MenuEnterHeld) {
        switch (currentSelection) {
        case 0: // Start Game
            choice = 1;
            break;
        case 1: // Options
            choice = 2;
            break;
        case 2: // Exit
            PostQuitMessage(0);
            break;
        }
    }
    g_MenuEnterHeld = enterPressed;
}

void ResetMenuInputState() {
    g_MenuUpHeld = (diKeys[DIK_UP] & 0x80) != 0 || (diKeys[DIK_W] & 0x80) != 0;
    g_MenuDownHeld = (diKeys[DIK_DOWN] & 0x80) != 0 || (diKeys[DIK_S] & 0x80) != 0;
    g_MenuEnterHeld = (diKeys[DIK_RETURN] & 0x80) != 0;
}

void mainMenu(int& choice) {
    if (!menuBackground && !menuFont) {
        LoadMenuTextures();
    }

    UpdateMenuInput(choice);
    renderMainMenu();
}