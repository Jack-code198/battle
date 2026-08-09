#include "Joker.h"
#include "JokerAssets.h"
#include "../../config.h"
#include "../../renderer.h"
#include "../../ui.h"
#include <cmath>
#include <stdio.h>

struct JokerTexture {
    LPDIRECT3DTEXTURE9 texture = nullptr;
    int maxFrame = 1;
    int cols = 1;
    int rows = 1;
};

struct JokerTextureSet {
    JokerTexture joker;
    JokerTexture arsene;
    JokerTexture jokerEffect;
    JokerTexture arseneEffect;
    bool pairedWithArsene = false;
};

static JokerTextureSet g_JokerAnims[JOKER_ANIM_COUNT];
static const int kJokerCellSize = MAKOTO_CELL_SIZE;

static bool LoadJokerSheet(JokerTexture& tex, const char* fileName, int frameCount) {
    if (!fileName) return true;

    char path[512];
    sprintf_s(path, "assets/joker/%s", fileName);

    HRESULT hr = D3DXCreateTextureFromFileEx(
        g_pD3DDevice,
        path,
        D3DX_DEFAULT_NONPOW2,
        D3DX_DEFAULT_NONPOW2,
        D3DX_DEFAULT,
        NULL,
        D3DFMT_A8R8G8B8,
        D3DPOOL_MANAGED,
        D3DX_DEFAULT,
        D3DX_DEFAULT,
        D3DCOLOR_XRGB(JOKER_COLORKEY_R, JOKER_COLORKEY_G, JOKER_COLORKEY_B),
        NULL,
        NULL,
        &tex.texture);

    if (FAILED(hr) || !tex.texture) {
        return false;
    }

    ApplyJokerColorKey(tex.texture);

    D3DSURFACE_DESC desc;
    tex.texture->GetLevelDesc(0, &desc);
    tex.cols = (int)(desc.Width / kJokerCellSize);
    tex.rows = (int)(desc.Height / kJokerCellSize);
    if (tex.cols < 1) tex.cols = 1;
    if (tex.rows < 1) tex.rows = 1;

    int gridFrames = tex.cols * tex.rows;
    tex.maxFrame = (frameCount > 0) ? frameCount : gridFrames;
    if (tex.maxFrame > gridFrames) tex.maxFrame = gridFrames;
    if (tex.maxFrame < 1) tex.maxFrame = 1;
    return true;
}

static void ReleaseJokerSheet(JokerTexture& tex) {
    if (tex.texture) {
        tex.texture->Release();
        tex.texture = nullptr;
    }
}

static void SetJokerFrameRect(RECT& rect, const JokerTexture& tex, int frameIndex) {
    int frame = frameIndex;
    if (frame < 0) frame = 0;
    if (frame >= tex.maxFrame) frame = tex.maxFrame - 1;
    rect.left = kJokerCellSize * (frame % tex.cols);
    rect.top = kJokerCellSize * (frame / tex.cols);
    rect.right = rect.left + kJokerCellSize;
    rect.bottom = rect.top + kJokerCellSize;
}

static void DrawJokerLayerSprite(
    LPD3DXSPRITE sprite,
    const JokerTexture& tex,
    int frameIndex,
    const D3DXVECTOR3& pos,
    int facingDirection,
    float bodyHeight,
    float feetY,
    D3DCOLOR color)
{
    if (!sprite || !tex.texture) return;
    RECT rect;
    SetJokerFrameRect(rect, tex, frameIndex);
    DrawScaledCharacterSprite(sprite, tex.texture, &rect, pos, facingDirection, 1.0f, color, bodyHeight, feetY);
}

Joker::Joker()
    : currentFrame(0), maxFrame(4), frameCounter(0), currentState(JOKER_STAND)
    , isStunned(false), isReturningToPosition(false)
    , damageAnimLength(4), damageTimer(0), stunTimer(0)
    , isDamageAnimating(false), isForceResetting(false), bForceReset(false)
    , shouldReturnToOriginal(false), isActive(true), trainingIdleFrames(0)
{
    originalPosition = D3DXVECTOR3(JOKER_SPAWN_X, CHARACTER_GROUND_Y, 0);
    position = originalPosition;
    facingDirection = -1;
    health = JOKER_MAX_HEALTH;
    maxHealth = JOKER_MAX_HEALTH;
    velocity = 4;
    isHit = false;
    hitStunTimer = 0;
    isDead = false;
    UpdateHurtbox();
}

void Joker::TryTrainingHeal() {
    if (!TRAINING_MODE || health >= maxHealth || isHit) return;

    trainingIdleFrames++;
    if (trainingIdleFrames < TRAINING_HEAL_IDLE_FRAMES) return;

    health = maxHealth;
    isDead = false;
    trainingIdleFrames = 0;
    SyncBattleHudHealth(2, health, maxHealth);
}

void Joker::UpdateHurtbox() {
    float s = GetJokerDrawScale();
    hurtbox.width = JOKER_HURTBOX_WIDTH * s;
    hurtbox.height = JOKER_HURTBOX_HEIGHT * s;
    hurtbox.x = position.x - hurtbox.width * 0.5f;
    hurtbox.y = position.y - hurtbox.height;
}

AABB Joker::GetBodyCollisionBox() {
    float s = GetJokerDrawScale();
    AABB box;
    box.width = JOKER_PUSHBOX_WIDTH * s;
    box.height = JOKER_PUSHBOX_HEIGHT * s;
    box.x = position.x - box.width * 0.5f;
    box.y = position.y - box.height;
    return box;
}

void Joker::ClampPosition() {
    if (std::isnan(position.x) || std::isnan(position.y) ||
        std::isinf(position.x) || std::isinf(position.y)) {
        position = originalPosition;
        UpdateHurtbox();
        return;
    }

    ClampJokerCenterX(position.x);

    if (position.y > CHARACTER_GROUND_Y + 20.0f) position.y = CHARACTER_GROUND_Y + 20.0f;
    if (position.y < 200.0f) position.y = 200.0f;
}

void Joker::ResetAllStates() {
    isHit = false;
    isStunned = false;
    isReturningToPosition = false;
    isDamageAnimating = false;
    bForceReset = false;
    damageTimer = 0;
    stunTimer = 0;
    currentFrame = 0;
    frameCounter = 0;
    currentState = JOKER_STAND;
    maxFrame = g_JokerAnims[JOKER_ANIM_STANCE].joker.maxFrame;
    isActive = true;
    shouldReturnToOriginal = false;
}

void Joker::BeginHitReaction(float knockbackX) {
    isHit = true;
    isStunned = true;
    shouldReturnToOriginal = false;
    currentState = JOKER_DAMAGE;
    currentFrame = 0;
    maxFrame = g_JokerAnims[JOKER_ANIM_DAMAGE].joker.maxFrame;
    isDamageAnimating = true;
    damageTimer = 0;
    stunTimer = 0;
    frameCounter = 0;
    isReturningToPosition = false;

    position.x += knockbackX;
    ClampPosition();
    UpdateHurtbox();
}

void Joker::ReturnToOriginalPosition() {
    float dx = originalPosition.x - position.x;
    float absDx = fabsf(dx);

    if (absDx <= 2.0f) {
        position = originalPosition;
        isReturningToPosition = false;
        shouldReturnToOriginal = false;
        currentState = JOKER_STAND;
        currentFrame = 0;
        maxFrame = g_JokerAnims[JOKER_ANIM_STANCE].joker.maxFrame;
        frameCounter = 0;
        UpdateHurtbox();
        return;
    }

    if (currentState != JOKER_WALK) {
        currentState = JOKER_WALK;
        currentFrame = 0;
        maxFrame = g_JokerAnims[JOKER_ANIM_WALK].joker.maxFrame;
        frameCounter = 0;
    }

    float speed = OPPONENT_RETURN_SPEED;
    if (absDx <= speed) {
        position.x = originalPosition.x;
    }
    else {
        position.x += (dx > 0.0f) ? speed : -speed;
    }
    position.y = originalPosition.y;

    ClampPosition();
    facingDirection = (dx > 0.0f) ? 1 : -1;

    frameCounter++;
    if (frameCounter > OPPONENT_WALK_ANIM_TICKS) {
        currentFrame = (currentFrame + 1) % maxFrame;
        frameCounter = 0;
    }
    UpdateHurtbox();
}

void Joker::Update() {
    if (isDead) return;

    if (isForceResetting || bForceReset) {
        position = originalPosition;
        isForceResetting = false;
        bForceReset = false;
        ResetAllStates();
        UpdateHurtbox();
        return;
    }

    if (isHit) {
        if (currentState != JOKER_DAMAGE) {
            currentState = JOKER_DAMAGE;
            currentFrame = 0;
            maxFrame = g_JokerAnims[JOKER_ANIM_DAMAGE].joker.maxFrame;
            damageTimer = 0;
            stunTimer = 0;
            frameCounter = 0;
        }

        stunTimer++;
        frameCounter++;
        if (frameCounter > OPPONENT_DAMAGE_ANIM_TICKS && currentFrame < maxFrame - 1) {
            currentFrame++;
            frameCounter = 0;
        }

        UpdateHurtbox();

        if (stunTimer >= 30) {
            isHit = false;
            isStunned = false;
            isDamageAnimating = false;
            damageTimer = 0;
            stunTimer = 0;
            shouldReturnToOriginal = true;
            isReturningToPosition = true;
            currentState = JOKER_STAND;
            currentFrame = 0;
            maxFrame = g_JokerAnims[JOKER_ANIM_STANCE].joker.maxFrame;
            frameCounter = 0;
            trainingIdleFrames = 0;
            UpdateHurtbox();
        }
        return;
    }

    if (isReturningToPosition || shouldReturnToOriginal) {
        if (JOKER_SANDBAG_MODE) {
            position = originalPosition;
            isReturningToPosition = false;
            shouldReturnToOriginal = false;
            currentState = JOKER_STAND;
            currentFrame = 0;
            maxFrame = g_JokerAnims[JOKER_ANIM_STANCE].joker.maxFrame;
            UpdateHurtbox();
        }
        else {
            ReturnToOriginalPosition();
        }
        return;
    }

    ClampPosition();

    if (currentState == JOKER_STAND) {
        facingDirection = -1;
        maxFrame = g_JokerAnims[JOKER_ANIM_STANCE].joker.maxFrame;
        frameCounter++;
        if (frameCounter > OPPONENT_STAND_ANIM_TICKS) {
            currentFrame = (currentFrame + 1) % maxFrame;
            frameCounter = 0;
        }
        TryTrainingHeal();
    }

    UpdateHurtbox();
}

void Joker::TakeDamage(int damage) {
    if (isDead || isHit) return;

    health -= damage;
    if (health < 0) health = 0;
    trainingIdleFrames = 0;

    float knockback = 0.0f;
    if (!JOKER_SANDBAG_MODE) {
        knockback = (facingDirection == 1) ? -OPPONENT_MELEE_KNOCKBACK : OPPONENT_MELEE_KNOCKBACK;
    }
    BeginHitReaction(knockback);

    if (!TRAINING_MODE && health <= 0) {
        isDead = true;
    }
}

void Joker::ApplySkillDamage(int damage) {
    if (isDead) return;

    health -= damage;
    if (health < 0) health = 0;
    trainingIdleFrames = 0;

    if (isHit) {
        return;
    }

    float knockback = 0.0f;
    if (!JOKER_SANDBAG_MODE) {
        knockback = (facingDirection == 1) ? -OPPONENT_SKILL_KNOCKBACK : OPPONENT_SKILL_KNOCKBACK;
    }
    BeginHitReaction(knockback);

    if (!TRAINING_MODE && health <= 0) {
        isDead = true;
    }
}

void Joker::Reset() {
    originalPosition = D3DXVECTOR3(JOKER_SPAWN_X, CHARACTER_GROUND_Y, 0);
    position = originalPosition;
    facingDirection = -1;
    health = maxHealth;
    sp = maxSp;
    isDead = false;
    isActive = true;
    trainingIdleFrames = 0;
    ResetAllStates();
    UpdateHurtbox();
}

static JokerAnimId GetAnimForState(int state) {
    switch (state) {
    case JOKER_WALK: return JOKER_ANIM_WALK;
    case JOKER_DAMAGE: return JOKER_ANIM_DAMAGE;
    case JOKER_STAND:
    default: return JOKER_ANIM_STANCE;
    }
}

void Joker::DrawArseneSprite(LPD3DXSPRITE sprite, JokerTexture& tex, int frame, D3DCOLOR color) const {
    D3DXVECTOR3 arsenePos(
        position.x - (float)facingDirection * ARSENE_BEHIND_HORIZONTAL,
        position.y - ARSENE_BEHIND_VERTICAL,
        0.0f);
    int arseneFrame = frame;
    if (tex.maxFrame > 0) {
        arseneFrame %= tex.maxFrame;
    }
    DrawJokerLayerSprite(sprite, tex, arseneFrame, arsenePos, facingDirection,
        ARSENE_BODY_HEIGHT, ARSENE_FEET_Y, color);
}

void Joker::DrawBodySprite(LPD3DXSPRITE sprite, JokerTexture& tex, int frame, const D3DXVECTOR3& pos, D3DCOLOR color) const {
    DrawJokerLayerSprite(sprite, tex, frame, pos, facingDirection, JOKER_BODY_HEIGHT, JOKER_FEET_Y, color);
}

void Joker::DrawEffectSprite(LPD3DXSPRITE sprite, JokerTexture& tex, int frame, const D3DXVECTOR3& pos, float bodyHeight, float feetY, D3DCOLOR color) const {
    DrawJokerLayerSprite(sprite, tex, frame, pos, facingDirection, bodyHeight, feetY, color);
}

void Joker::Render(LPD3DXSPRITE sprite) {
    UpdateHurtbox();

    const JokerAnimId animId = GetAnimForState(currentState);
    const JokerTextureSet& set = g_JokerAnims[animId];
    if (!set.joker.texture) return;

    D3DCOLOR color = isHit ? D3DCOLOR_XRGB(255, 100, 100) : D3DCOLOR_XRGB(255, 255, 255);
    int bodyFrame = currentFrame;
    if (set.joker.maxFrame > 0) {
        if (bodyFrame < 0) bodyFrame = 0;
        if (bodyFrame >= set.joker.maxFrame) bodyFrame = set.joker.maxFrame - 1;
    }

    if (set.pairedWithArsene && set.arsene.texture && currentState != JOKER_DAMAGE) {
        if (set.arseneEffect.texture) {
            D3DXVECTOR3 arseneEffectPos(
                position.x - (float)facingDirection * ARSENE_BEHIND_HORIZONTAL,
                position.y - ARSENE_BEHIND_VERTICAL,
                0.0f);
            DrawEffectSprite(sprite, const_cast<JokerTexture&>(set.arseneEffect), bodyFrame,
                arseneEffectPos, ARSENE_BODY_HEIGHT, ARSENE_FEET_Y, color);
        }
        DrawArseneSprite(sprite, const_cast<JokerTexture&>(set.arsene), bodyFrame, color);
    }

    if (set.jokerEffect.texture) {
        DrawEffectSprite(sprite, const_cast<JokerTexture&>(set.jokerEffect), bodyFrame,
            position, JOKER_BODY_HEIGHT, JOKER_FEET_Y, color);
    }

    DrawBodySprite(sprite, const_cast<JokerTexture&>(set.joker), bodyFrame, position, color);
}

AABB Joker::GetHurtbox() {
    UpdateHurtbox();
    return hurtbox;
}

void Joker::RenderDebugHitbox(LPD3DXSPRITE sprite) {
    if (!sprite) return;
    UpdateHurtbox();
    DrawDebugRect(sprite, hurtbox.x, hurtbox.y, hurtbox.width, hurtbox.height, D3DCOLOR_ARGB(160, 255, 64, 64));
}

bool LoadJokerTextures() {
    for (int i = 0; i < JOKER_ANIM_COUNT; ++i) {
        const JokerAnimInfo& info = kJokerAnimCatalog[i];
        JokerTextureSet& set = g_JokerAnims[i];
        set.pairedWithArsene = info.pairedWithArsene;

        if (info.jokerFile && !LoadJokerSheet(set.joker, info.jokerFile, info.frameCount)) {
            return false;
        }
        if (info.arseneFile) {
            LoadJokerSheet(set.arsene, info.arseneFile, info.frameCount);
        }
        if (info.jokerEffectFile) {
            LoadJokerSheet(set.jokerEffect, info.jokerEffectFile, info.frameCount);
        }
        if (info.arseneEffectFile) {
            LoadJokerSheet(set.arseneEffect, info.arseneEffectFile, info.frameCount);
        }
    }
    return g_JokerAnims[JOKER_ANIM_STANCE].joker.texture != nullptr;
}

void CleanUpJokerTextures() {
    for (int i = 0; i < JOKER_ANIM_COUNT; ++i) {
        ReleaseJokerSheet(g_JokerAnims[i].joker);
        ReleaseJokerSheet(g_JokerAnims[i].arsene);
        ReleaseJokerSheet(g_JokerAnims[i].jokerEffect);
        ReleaseJokerSheet(g_JokerAnims[i].arseneEffect);
    }
}

const JokerTextureSet* GetJokerAnimSet(JokerAnimId id) {
    if (id < 0 || id >= JOKER_ANIM_COUNT) return nullptr;
    return &g_JokerAnims[id];
}
