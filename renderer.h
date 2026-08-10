#pragma once
#include "config.h"

// 2D rendering module (BMCS2224) - Direct3D 9 device + D3DX sprite present path.

bool InitD3D();
void Render();
void CleanUpD3D();
void ToggleFullscreen();
bool IsFullscreen();

extern IDirect3DDevice9* g_pD3DDevice;
extern LPD3DXSPRITE spriteBrush;
