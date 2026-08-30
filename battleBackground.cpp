#include "BattleBackground.h"
#include "GameLogic.h"
#include "player/Fighter.h"
#include <d3dx9.h>
#include <cmath>
#include <cstring>

extern LPDIRECT3DTEXTURE9 texBgCity1;

struct ParallaxLayerDef {
    const char* path;
    float factor;
};

struct ParallaxStageDef {
    const ParallaxLayerDef* layers;
    int layerCount;
};

static const ParallaxLayerDef kCity3Layers[] = {
    { "assets/background/City3/sky.png", 0.04f },
    { "assets/background/City3/houses3.png", 0.10f },
    { "assets/background/City3/houded2.png", 0.18f },
    { "assets/background/City3/houses1.png", 0.26f },
    { "assets/background/City3/road.png", 0.40f },
    { "assets/background/City3/crosswalk.png", 0.55f },
};

static const ParallaxLayerDef kCity4Layers[] = {
    { "assets/background/City4/Sky.png", 0.04f },
    { "assets/background/City4/houses.png", 0.10f },
    { "assets/background/City4/houses2.png", 0.16f },
    { "assets/background/City4/houses1.png", 0.24f },
    { "assets/background/City4/road.png", 0.36f },
    { "assets/background/City4/fountain&bush.png", 0.48f },
    { "assets/background/City4/umbrella&policebox.png", 0.58f },
};

static const ParallaxLayerDef kCity2Layers[] = {
    { "assets/background/City2/Sky.png", 0.04f },
    { "assets/background/City2/back.png", 0.08f },
    { "assets/background/City2/houses3.png", 0.14f },
    { "assets/background/City2/houses1.png", 0.22f },
    { "assets/background/City2/road&lamps.png", 0.36f },
    { "assets/background/City2/minishop&callbox.png", 0.52f },
};

static const ParallaxStageDef kParallaxStages[] = {
    { kCity3Layers, (int)(sizeof(kCity3Layers) / sizeof(kCity3Layers[0])) },
    { kCity4Layers, (int)(sizeof(kCity4Layers) / sizeof(kCity4Layers[0])) },
    { kCity2Layers, (int)(sizeof(kCity2Layers) / sizeof(kCity2Layers[0])) },
};

struct LoadedParallaxLayer {
    LPDIRECT3DTEXTURE9 texture = nullptr;
    float factor = 0.0f;
};

static LoadedParallaxLayer g_ParallaxLayers[BATTLE_PARALLAX_MAX_LAYERS] = {};
static int g_ParallaxLayerCount = 0;
static float g_ParallaxScroll = 0.0f;
static float g_ParallaxAnchorX = 0.0f;

static Fighter* GetHumanFighter() {
    if (g_Player1 && g_Player1->IsHumanControlled()) return g_Player1;
    if (g_Player2 && g_Player2->IsHumanControlled()) return g_Player2;
    return g_Player1;
}

// Practical 11-style positive modulo for seamless horizontal tiling.
static float PositiveMod(float value, float period) {
    if (period <= 0.0f) return 0.0f;
    float mod = fmodf(value, period);
    if (mod < 0.0f) mod += period;
    return mod;
}

// Draw one layer three times so the viewport never shows empty edges while scrolling.
static void DrawLoopingParallaxLayer(
    LPD3DXSPRITE sprite,
    LPDIRECT3DTEXTURE9 texture,
    float scaleX,
    float scaleY,
    float layerScroll)
{
    const float tileW = (float)SCREEN_WIDTH;
    const float baseX = -PositiveMod(layerScroll, tileW);

    for (int copy = 0; copy < BATTLE_PARALLAX_LOOP_COPIES; ++copy) {
        const float drawX = baseX + (float)copy * tileW;

        D3DXMATRIX matScale;
        D3DXMATRIX matTrans;
        D3DXMATRIX matFinal;
        D3DXMatrixScaling(&matScale, scaleX, scaleY, 1.0f);
        D3DXMatrixTranslation(&matTrans, drawX, 0.0f, 0.0f);
        matFinal = matScale * matTrans;
        sprite->SetTransform(&matFinal);

        D3DXVECTOR3 pos(0.0f, 0.0f, 0.0f);
        sprite->Draw(texture, NULL, NULL, &pos, D3DCOLOR_XRGB(255, 255, 255));
    }
}

static void ReleaseParallaxLayers() {
    for (int i = 0; i < g_ParallaxLayerCount; ++i) {
        if (g_ParallaxLayers[i].texture) {
            g_ParallaxLayers[i].texture->Release();
            g_ParallaxLayers[i].texture = nullptr;
        }
    }
    g_ParallaxLayerCount = 0;
}

static bool LoadTextureFromFile(const char* path, LPDIRECT3DTEXTURE9* outTexture) {
    if (!g_pD3DDevice || !path || !outTexture) return false;

    HRESULT hr = D3DXCreateTextureFromFileEx(
        g_pD3DDevice,
        path,
        D3DX_DEFAULT_NONPOW2,
        D3DX_DEFAULT_NONPOW2,
        D3DX_DEFAULT,
        0,
        D3DFMT_A8R8G8B8,
        D3DPOOL_MANAGED,
        D3DX_DEFAULT,
        D3DX_DEFAULT,
        0,
        NULL,
        NULL,
        outTexture);

    return SUCCEEDED(hr) && *outTexture != nullptr;
}

bool LoadBattleParallaxForStage(int stageIndex) {
    ReleaseParallaxLayers();

    if (stageIndex < 0 || stageIndex >= (int)(sizeof(kParallaxStages) / sizeof(kParallaxStages[0]))) {
        return false;
    }

    const ParallaxStageDef& stage = kParallaxStages[stageIndex];
    const int layerCap = (int)(sizeof(g_ParallaxLayers) / sizeof(g_ParallaxLayers[0]));
    int loaded = 0;

    for (int i = 0; i < stage.layerCount && loaded < layerCap; ++i) {
        LPDIRECT3DTEXTURE9 texture = nullptr;
        if (!LoadTextureFromFile(stage.layers[i].path, &texture)) {
            continue;
        }

        g_ParallaxLayers[loaded].texture = texture;
        g_ParallaxLayers[loaded].factor = stage.layers[i].factor;
        ++loaded;
    }

    g_ParallaxLayerCount = loaded;
    return loaded > 0;
}

void CleanUpBattleParallax() {
    ReleaseParallaxLayers();
    g_ParallaxScroll = 0.0f;
    g_ParallaxAnchorX = 0.0f;
}

void ResetBattleParallaxScroll() {
    g_ParallaxScroll = 0.0f;
    if (Fighter* human = GetHumanFighter()) {
        g_ParallaxAnchorX = human->GetPosition().x;
    }
    else {
        g_ParallaxAnchorX = (float)SCREEN_WIDTH * 0.5f;
    }
}

void UpdateBattleParallaxScroll() {
    if (g_ParallaxLayerCount <= 0) return;
    if (!IsBattleCombatActive()) return;

    Fighter* human = GetHumanFighter();
    if (!human) return;

    const float targetScroll =
        (human->GetPosition().x - g_ParallaxAnchorX) * BATTLE_PARALLAX_SENSITIVITY;
    g_ParallaxScroll += (targetScroll - g_ParallaxScroll) * BATTLE_PARALLAX_LERP;
}

bool BattleParallaxBackgroundReady() {
    return g_ParallaxLayerCount > 0;
}

static void DrawFallbackBackground(LPD3DXSPRITE sprite) {
    if (!sprite || !texBgCity1) return;

    D3DSURFACE_DESC desc;
    texBgCity1->GetLevelDesc(0, &desc);
    if (desc.Width == 0 || desc.Height == 0) return;

    const float scaleX = (float)SCREEN_WIDTH / (float)desc.Width;
    const float scaleY = (float)SCREEN_HEIGHT / (float)desc.Height;
    DrawLoopingParallaxLayer(sprite, texBgCity1, scaleX, scaleY, g_ParallaxScroll);
}

void DrawBattleParallaxBackground(LPD3DXSPRITE sprite) {
    if (!sprite) return;

    if (g_ParallaxLayerCount <= 0) {
        DrawFallbackBackground(sprite);
        return;
    }

    D3DSURFACE_DESC desc;
    g_ParallaxLayers[0].texture->GetLevelDesc(0, &desc);
    if (desc.Width == 0 || desc.Height == 0) {
        DrawFallbackBackground(sprite);
        return;
    }

    const float scaleX = (float)SCREEN_WIDTH / (float)desc.Width;
    const float scaleY = (float)SCREEN_HEIGHT / (float)desc.Height;

    for (int i = 0; i < g_ParallaxLayerCount; ++i) {
        if (!g_ParallaxLayers[i].texture) continue;

        const float layerScroll = g_ParallaxScroll * g_ParallaxLayers[i].factor;
        DrawLoopingParallaxLayer(
            sprite,
            g_ParallaxLayers[i].texture,
            scaleX,
            scaleY,
            layerScroll);
    }

    D3DXMATRIX matIdentity;
    D3DXMatrixIdentity(&matIdentity);
    sprite->SetTransform(&matIdentity);
}
