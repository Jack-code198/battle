#pragma once
#include "Config.h"

// Thin wrapper around ID3DXFont for HUD / debug text (BMCS2224 Font requirement).
class FontRenderer {
public:
    FontRenderer();
    ~FontRenderer();

    bool Create(LPDIRECT3DDEVICE9 device, const char* fontFile, const char* family, int height);
    void Release();
    void OnLostDevice();
    void OnResetDevice();

    void DrawTextA(
        const char* text,
        float x,
        float y,
        D3DCOLOR color,
        bool rightAlign = false,
        float rightEdgeX = 0.0f) const;

    // Center text within a screen rectangle (game over / titles).
    void DrawTextCenteredInRect(
        const char* text,
        LONG left,
        LONG top,
        LONG right,
        LONG bottom,
        D3DCOLOR color) const;

    ID3DXFont* GetFont() const { return font; }
    bool IsReady() const { return font != nullptr; }

private:
    ID3DXFont* font;
    const char* loadedFontPath;
    bool fontResourceLoaded;
};

// D3D9 device lost/reset hooks (called from renderer.cpp ToggleFullscreen).
void BindGameFontRenderer(FontRenderer* renderer);
void NotifyGameFontDeviceLost();
void NotifyGameFontDeviceReset(LPDIRECT3DDEVICE9 device);
