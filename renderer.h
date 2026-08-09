#pragma once
#include "config.h"

bool InitD3D();
void Render();
void CleanUpD3D();
void ToggleFullscreen();
bool IsFullscreen();

extern IDirect3DDevice9* g_pD3DDevice;
extern LPD3DXSPRITE spriteBrush;