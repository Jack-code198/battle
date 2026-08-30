#include "FontRenderer.h"
#include <cstdio>

FontRenderer::FontRenderer()
    : font(nullptr)
    , loadedFontPath(nullptr)
    , fontResourceLoaded(false) {
}

FontRenderer::~FontRenderer() {
    Release();
}

bool FontRenderer::Create(LPDIRECT3DDEVICE9 device, const char* fontFile, const char* family, int height) {
    Release();
    if (!device || !fontFile || !family) return false;

    if (AddFontResourceExA(fontFile, FR_PRIVATE, 0) > 0) {
        loadedFontPath = fontFile;
        fontResourceLoaded = true;
    }

    HRESULT hr = D3DXCreateFontA(
        device,
        height,
        0,
        FW_BOLD,
        1,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        family,
        &font);

    return SUCCEEDED(hr) && font != nullptr;
}

void FontRenderer::Release() {
    if (font) {
        font->Release();
        font = nullptr;
    }
    if (fontResourceLoaded && loadedFontPath) {
        RemoveFontResourceExA(loadedFontPath, FR_PRIVATE, 0);
        loadedFontPath = nullptr;
        fontResourceLoaded = false;
    }
}

void FontRenderer::OnLostDevice() {
    if (font) font->OnLostDevice();
}

void FontRenderer::OnResetDevice() {
    if (!font) return;
    if (FAILED(font->OnResetDevice())) {
        Release();
    }
}

void FontRenderer::DrawTextA(
    const char* text,
    float x,
    float y,
    D3DCOLOR color,
    bool rightAlign,
    float rightEdgeX) const
{
    if (!font || !text) return;

    RECT rect;
    if (rightAlign) {
        rect.left = 0;
        rect.top = (LONG)y;
        rect.right = (LONG)rightEdgeX;
        rect.bottom = (LONG)(y + FONT_DRAW_LINE_HEIGHT);
        if (FAILED(font->DrawTextA(nullptr, text, -1, &rect, DT_RIGHT | DT_NOCLIP, color))) {
            return;
        }
    }
    else {
        rect.left = (LONG)x;
        rect.top = (LONG)y;
        rect.right = (LONG)(x + FONT_DRAW_MAX_WIDTH);
        rect.bottom = (LONG)(y + FONT_DRAW_LINE_HEIGHT);
        if (FAILED(font->DrawTextA(nullptr, text, -1, &rect, DT_LEFT | DT_NOCLIP, color))) {
            return;
        }
    }
}

void FontRenderer::DrawTextCenteredInRect(
    const char* text,
    LONG left,
    LONG top,
    LONG right,
    LONG bottom,
    D3DCOLOR color) const
{
    if (!font || !text) return;

    RECT rect = { left, top, right, bottom };
    font->DrawTextA(
        nullptr,
        text,
        -1,
        &rect,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP,
        color);
}

static FontRenderer* g_GameFontRenderer = nullptr;

void BindGameFontRenderer(FontRenderer* renderer) {
    g_GameFontRenderer = renderer;
}

void NotifyGameFontDeviceLost() {
    if (g_GameFontRenderer) {
        g_GameFontRenderer->OnLostDevice();
    }
}

void NotifyGameFontDeviceReset(LPDIRECT3DDEVICE9 device) {
    if (!g_GameFontRenderer || !device) return;
    g_GameFontRenderer->OnResetDevice();
    if (!g_GameFontRenderer->IsReady()) {
        g_GameFontRenderer->Create(device, HUD_FONT_FILE, HUD_FONT_FAMILY, 20);
    }
}
