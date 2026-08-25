#pragma once
#include "config.h"

// =============================================================================
// 2D rendering module (BMCS2224) — Direct3D 9 + ID3DXSprite.
// Device create/reset, fullscreen toggle, battle present. Sprite RECTs are
// computed from sheet cell size / cols / frame index (formula, not hardcoded).
// =============================================================================

bool InitD3D();
void Render();
void RenderBattleSceneContents();
void ApplyBrightnessOverlay();
void WarmupRenderPipeline();
void CleanUpD3D();
void ToggleFullscreen();
void ProcessGraphicsDeviceEvents();
bool IsFullscreen();

extern IDirect3DDevice9* g_pD3DDevice;
extern LPD3DXSPRITE spriteBrush;
