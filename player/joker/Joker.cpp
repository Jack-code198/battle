#include "Joker.h"
#include "JokerAssets.h"
#include "../../ai.h"
#include "../../config.h"
#include "../../renderer.h"
#include "../../ui.h"
#include "../../game_logic.h"
#include "../../input.h"
#include <cmath>
#include <optional>
#include <stdio.h>

extern AttackData attackHitbox;
extern AttackData sideAttackHitbox;
extern AttackData attackUpHitbox;
extern AttackData downAttackHitbox;


// Same stepping helpers Makoto uses so Joker animates on the shared 60 FPS timer.
// Returns true only after the last frame has also been held for ticksPerFrame
// (so short sheets like recover's 3 frames are actually visible).
static bool AdvanceOneShotFrame(int& accumulator, int& frame, int steps, int ticksPerFrame, int maxFrame) {
    if (maxFrame <= 0 || ticksPerFrame <= 0) return false;
    accumulator += steps;
    while (accumulator >= ticksPerFrame) {
        accumulator -= ticksPerFrame;
        if (frame < maxFrame - 1) {
            frame++;
        }
        else {
            return true;
        }
    }
    return false;
}

static void AdvanceLoopFrame(int& accumulator, int& frame, int steps, int ticksPerFrame, int frameCount) {
    if (frameCount <= 0) return;
    accumulator += steps;
    while (accumulator >= ticksPerFrame) {
        accumulator -= ticksPerFrame;
        frame = (frame + 1) % frameCount;
    }
}

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
static JokerTexture g_MonaWin;
static const int kJokerCellSize = MAKOTO_CELL_SIZE;

static JokerAnimId GetAnimForState(int state);

static int GetMaxFrameForState(int state) {
    const JokerAnimId animId = GetAnimForState(state);
    int frames = g_JokerAnims[animId].joker.maxFrame;
    if (frames < 1) frames = 1;
    return frames;
}

static bool IsMeleeAttackState(int state) {
    switch (state) {
    case JOKER_ATTACK:
    case JOKER_FORWARD_ATTACK:
    case JOKER_UP_ATTACK:
    case JOKER_DOWN_ATTACK:
    case JOKER_FORWARD_SMASH:
    case JOKER_UP_SMASH:
    case JOKER_DOWN_SMASH:
    case JOKER_NEUTRAL_AIR:
    case JOKER_FORWARD_AIR:
    case JOKER_BACK_AIR:
    case JOKER_DOWN_AIR:
    case JOKER_UP_AIR:
    case JOKER_DASH:
        return true;
    default:
        return false;
    }
}

static bool IsAirAttackState(int state) {
    switch (state) {
    case JOKER_NEUTRAL_AIR:
    case JOKER_FORWARD_AIR:
    case JOKER_BACK_AIR:
    case JOKER_DOWN_AIR:
    case JOKER_UP_AIR:
        return true;
    default:
        return false;
    }
}

static bool IsSkillState(int state) {
    switch (state) {
    case JOKER_EIHA:
    case JOKER_EIGAON:
    case JOKER_NEUTRAL_SPECIAL:
    case JOKER_NEUTRAL_AIR_SPECIAL:
    case JOKER_ALL_OUT_EFFECT: // only the effect phase deals skill damage
        return true;
    default:
        return false;
    }
}

static bool IsAllOutPhase(int state) {
    return state == JOKER_ALL_OUT_ATTACK ||
        state == JOKER_ALL_OUT_MEMBER ||
        state == JOKER_ALL_OUT_EFFECT ||
        state == JOKER_ALL_OUT_FINISH;
}

static int GetSkillDamageForState(int state) {
    switch (state) {
    case JOKER_EIHA: return 40;
    case JOKER_EIGAON: return 55;
    case JOKER_NEUTRAL_SPECIAL:
    case JOKER_NEUTRAL_AIR_SPECIAL: return 50;
    case JOKER_ALL_OUT_EFFECT: return 90;
    default: return 0;
    }
}

static int GetSkillHitStartFrame(int state) {
    switch (state) {
    case JOKER_EIHA:
    case JOKER_EIGAON: return 1;
    case JOKER_NEUTRAL_SPECIAL:
    case JOKER_NEUTRAL_AIR_SPECIAL: return 2;
    case JOKER_ALL_OUT_EFFECT: return 1;
    default: return 1;
    }
}

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

// Build a source RECT from Joker's grid sprite sheet (same formula as Makoto).
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
    : currentFrame(0), maxFrame(1), frameCounter(0), animAccumulator(0), currentState(JOKER_INTRO)
    , isStunned(false), isReturningToPosition(false)
    , damageAnimLength(4), damageTimer(0), stunTimer(0)
    , isDamageAnimating(false), isForceResetting(false), forceResetRequested(false)
    , shouldReturnToOriginal(false), isActive(true), trainingIdleFrames(0), idleWaitFrames(0)
    , damageGroundHold(0), introDisplayHold(0), introLastFrame(0)
    , jumpCount(0), jumpHorizontalSpeed(0.0f), verticalVelocity(0.0f)
    , hitThisAttack(false), attackButtonHeld(false), dodgeForward(true)
    , skillHit(false), skillEffectFrame(0), skillEffectAccum(0)
    , skillEffectPos(0.0f, 0.0f, 0.0f)
{
    SetCharacterId(Char_Joker);
    ApplySlotSpawnDefaults();
    originalPosition = position;
    health = JOKER_MAX_HEALTH;
    maxHealth = JOKER_MAX_HEALTH;
    velocity = JOKER_MOVE_SPEED;
    isHit = false;
    hitStunTimer = 0;
    isDead = false;
    maxFrame = g_JokerAnims[JOKER_ANIM_INTRO].joker.maxFrame;
    if (maxFrame < 1) maxFrame = 1;
    UpdateHurtbox();
}

void Joker::TryTrainingHeal() {
    if (!IsTutorialBattleMode() || health >= maxHealth || isHit) return;

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

AABB Joker::GetBodyCollisionBox() const {
    return MakeLivePushbox(position, facingDirection, JOKER_PUSHBOX_WIDTH, JOKER_PUSHBOX_HEIGHT, GetJokerDrawScale());
}

void Joker::UpdateScaledHurtbox() {
    UpdateHurtbox();
}

bool Joker::IsOnGround() const {
    return position.y >= CHARACTER_GROUND_Y - GROUND_CONTACT_EPSILON;
}

void Joker::ApplyGravity(int steps) {
    // BMCS2224 Physics module: force → acceleration → velocity → position.
    ApplyPhysicsGravitySteps(steps, verticalVelocity);
}

void Joker::ClampPosition() {
    if (std::isnan(position.x) || std::isnan(position.y) ||
        std::isinf(position.x) || std::isinf(position.y)) {
        position = originalPosition;
        UpdateHurtbox();
        return;
    }

    ClampJokerCenterX(position.x);

    if (position.y > CHARACTER_GROUND_Y + JOKER_MAX_GROUND_SLACK) {
        position.y = CHARACTER_GROUND_Y + JOKER_MAX_GROUND_SLACK;
    }
    if (position.y < JOKER_MIN_SCREEN_Y) {
        position.y = JOKER_MIN_SCREEN_Y;
    }
}

void Joker::EnterStance() {
    isHit = false;
    isStunned = false;
    isDamageAnimating = false;
    currentState = JOKER_STAND;
    currentFrame = 0;
    frameCounter = 0;
    animAccumulator = 0;
    idleWaitFrames = 0;
    maxFrame = g_JokerAnims[JOKER_ANIM_STANCE].joker.maxFrame;
    if (maxFrame < 1) maxFrame = 1;
}

void Joker::EnterIdle() {
    // If idle sheet failed to load, keep stance instead of disappearing.
    if (!g_JokerAnims[JOKER_ANIM_IDLE].joker.texture) {
        idleWaitFrames = 0;
        return;
    }

    currentState = JOKER_IDLE;
    currentFrame = 0;
    frameCounter = 0;
    animAccumulator = 0;
    idleWaitFrames = 0;
    maxFrame = g_JokerAnims[JOKER_ANIM_IDLE].joker.maxFrame;
    if (maxFrame < 1) maxFrame = g_JokerAnims[JOKER_ANIM_STANCE].joker.maxFrame;
    if (maxFrame < 1) maxFrame = 1;
}

void Joker::EnterActionState(int state) {
    currentState = state;
    currentFrame = 0;
    frameCounter = 0;
    animAccumulator = 0;
    hitThisAttack = false;
    skillHit = false;
    skillEffectFrame = 0;
    skillEffectAccum = 0;
    maxFrame = GetMaxFrameForState(state);
}

void Joker::UpdateSkillHits(Fighter& enemy, int steps) {
    if (!IsSkillState(currentState) || enemy.isDead) return;

    // All-out / persona skills yank the foe in for the cinematic hit.
    if (IsAllOutPhase(currentState) ||
        currentState == JOKER_EIHA ||
        currentState == JOKER_EIGAON) {
        PullEnemyForUltimate(*this, enemy, !skillHit && !enemy.IsHit());
    }

    const AABB& hb = enemy.GetHurtbox();
    // Makoto GetAgiMabufuPos: effect feet sit on enemy hurtbox center.
    skillEffectPos = D3DXVECTOR3(hb.x + hb.width * 0.5f, hb.y + hb.height * 0.5f, 0.0f);

    const JokerAnimId animId = GetAnimForState(currentState);
    const JokerTextureSet& set = g_JokerAnims[animId];
    const int effectMax = set.jokerEffect.texture
        ? set.jokerEffect.maxFrame
        : (set.arseneEffect.texture ? set.arseneEffect.maxFrame : maxFrame);
    if (effectMax < 1) return;

    const int effectTicks =
        (currentState == JOKER_EIHA || currentState == JOKER_EIGAON)
        ? LASER_EFFECT_ANIM_DELAY
        : PERSONA_EFFECT_ANIM_DELAY;

    skillEffectAccum += steps;
    while (skillEffectAccum >= effectTicks) {
        skillEffectAccum -= effectTicks;
        if (skillEffectFrame < effectMax - 1) {
            skillEffectFrame++;
            skillEndHold = 0;
        }
        else {
            skillEndHold += effectTicks;
        }
    }

    if (!skillHit) {
        const int hitStart = GetSkillHitStartFrame(currentState);
        const bool effectReady = skillEffectFrame >= hitStart || currentFrame >= hitStart;
        if (effectReady) {
            enemy.ApplySkillDamage(GetSkillDamageForState(currentState));
            skillHit = true;
        }
    }
}

void Joker::ResetAllStates() {
    isHit = false;
    isStunned = false;
    isReturningToPosition = false;
    isDamageAnimating = false;
    forceResetRequested = false;
    damageTimer = 0;
    stunTimer = 0;
    currentFrame = 0;
    frameCounter = 0;
    animAccumulator = 0;
    idleWaitFrames = 0;
    damageGroundHold = 0;
    introDisplayHold = 0;
    introLastFrame = 0;
    currentState = JOKER_INTRO;
    maxFrame = g_JokerAnims[JOKER_ANIM_INTRO].joker.maxFrame;
    if (maxFrame < 1) maxFrame = 1;
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
    if (maxFrame < 1) maxFrame = 1;
    isDamageAnimating = true;
    damageTimer = 0;
    stunTimer = 0;
    frameCounter = 0;
    animAccumulator = 0;
    idleWaitFrames = 0;
    damageGroundHold = 0;
    isReturningToPosition = false;

    TryApplyHorizontalDelta(knockbackX);
    ApplyStandardHitReactionVertical(position, verticalVelocity, IsOnGround());
    if (IsOnGround()) {
        currentFrame = maxFrame - 1;
    }
    ClampPosition();
    UpdateHurtbox();
}

void Joker::BeginRecover() {
    isHit = false;
    isStunned = false;
    isDamageAnimating = false;
    damageTimer = 0;
    stunTimer = 0;
    currentState = JOKER_RECOVER;
    currentFrame = 0;
    frameCounter = 0;
    animAccumulator = 0;
    maxFrame = g_JokerAnims[JOKER_ANIM_RECOVER].joker.maxFrame;
    // Texture missing or empty sheet → skip straight to stance.
    if (!g_JokerAnims[JOKER_ANIM_RECOVER].joker.texture || maxFrame < 1) {
        FinishRecoverToStance();
        return;
    }
}

void Joker::FinishRecoverToStance() {
    trainingIdleFrames = 0;
    idleWaitFrames = 0;

    if (!IsHumanControlled()) {
        // CPU AI: stay at knockdown position (no sandbag spawn snap).
        isReturningToPosition = false;
        shouldReturnToOriginal = false;
        EnterStance();
        UpdateHurtbox();
        return;
    }

    shouldReturnToOriginal = true;
    isReturningToPosition = true;
    EnterStance();
    UpdateHurtbox();
}

void Joker::ReturnToOriginalPosition() {
    float dx = originalPosition.x - position.x;
    float absDx = fabsf(dx);

    if (absDx <= 2.0f) {
        position = originalPosition;
        isReturningToPosition = false;
        shouldReturnToOriginal = false;
        EnterStance();
        UpdateHurtbox();
        return;
    }

    if (currentState != JOKER_WALK) {
        currentState = JOKER_WALK;
        currentFrame = 0;
        maxFrame = g_JokerAnims[JOKER_ANIM_WALK].joker.maxFrame;
        if (maxFrame < 1) maxFrame = 1;
        frameCounter = 0;
    }

    float speed = OPPONENT_RETURN_SPEED;
    if (absDx <= speed) {
        position.x = originalPosition.x;
    }
    else {
        TryApplyHorizontalDelta((dx > 0.0f) ? speed : -speed);
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

void Joker::CheckAttackCollision(Fighter& enemy) {
    if (enemy.IsDead()) return;
    if (!IsMeleeAttackState(currentState)) return;

    if (currentFrame == 0) hitThisAttack = false;

    const AttackData* data = nullptr;
    switch (currentState) {
    case JOKER_ATTACK:
    case JOKER_NEUTRAL_AIR:
    case JOKER_DASH:
        data = &attackHitbox;
        break;
    case JOKER_FORWARD_ATTACK:
    case JOKER_FORWARD_SMASH:
    case JOKER_FORWARD_AIR:
    case JOKER_BACK_AIR:
        data = &sideAttackHitbox;
        break;
    case JOKER_UP_ATTACK:
    case JOKER_UP_SMASH:
    case JOKER_UP_AIR:
        data = &attackUpHitbox;
        break;
    case JOKER_DOWN_ATTACK:
    case JOKER_DOWN_SMASH:
    case JOKER_DOWN_AIR:
        data = &downAttackHitbox;
        break;
    default:
        return;
    }

    const float scale = GetJokerDrawScale();
    const int totalFrames = maxFrame > 0 ? maxFrame : data->endFrame;
    const int startF = (data->startFrame > 0) ? data->startFrame : 1;
    const int endF = (data->endFrame < totalFrames) ? data->endFrame : (totalFrames - 1);

    if (currentFrame < startF || currentFrame > endF) return;
    if (hitThisAttack) return;

    float boxW = data->width * scale;
    float boxH = data->height * scale;
    float forward = data->offsetX * scale;
    float attackX = (facingDirection == 1)
        ? (position.x + forward)
        : (position.x - forward - boxW);
    float attackY = position.y + data->offsetY * scale;
    const AABB attackBox = { attackX, attackY, boxW, boxH };

    if (CollisionHelper::AABBIntersect(attackBox, enemy.GetHurtbox())) {
        enemy.TakeDamage(data->damage);
        hitThisAttack = true;
        RestoreSp(SP_GAIN_ON_HIT);
    }
}

void Joker::UpdateSandbag(int steps) {
    const int animSteps = 1;

    if (isForceResetting || forceResetRequested) {
        position = originalPosition;
        isForceResetting = false;
        forceResetRequested = false;
        ResetAllStates();
        UpdateHurtbox();
        return;
    }

    // Entry intro plays once when battle Update starts, then goes to stance.
    if (currentState == JOKER_INTRO) {
        if (Fighter* opponent = GetOpponent(*this)) {
            facingDirection = (opponent->GetPosition().x >= position.x) ? 1 : -1;
        }
        else {
            facingDirection = -1;
        }
        maxFrame = g_JokerAnims[JOKER_ANIM_INTRO].joker.maxFrame;
        if (maxFrame < 1) {
            EnterStance();
            UpdateHurtbox();
            return;
        }
        if (introDisplayHold > 0) {
            currentFrame = introLastFrame;
            introDisplayHold -= animSteps;
            if (introDisplayHold <= 0) {
                introDisplayHold = 0;
                EnterStance();
            }
            UpdateHurtbox();
            return;
        }
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, JOKER_INTRO_TICKS, maxFrame)) {
            introLastFrame = (maxFrame > 0) ? (maxFrame - 1) : 0;
            currentFrame = introLastFrame;
            introDisplayHold = INTRO_END_HOLD_FRAMES;
        }
        UpdateHurtbox();
        return;
    }

    if (currentState == JOKER_DAMAGE || isHit) {
        if (currentState != JOKER_DAMAGE) {
            currentState = JOKER_DAMAGE;
            currentFrame = 0;
            maxFrame = g_JokerAnims[JOKER_ANIM_DAMAGE].joker.maxFrame;
            if (maxFrame < 1) maxFrame = 1;
            damageTimer = 0;
            stunTimer = 0;
            frameCounter = 0;
            animAccumulator = 0;
            damageGroundHold = 0;
        }

        ApplyGravity(steps);
        PinFighterToGround(position, verticalVelocity);
        stunTimer += steps;
        maxFrame = g_JokerAnims[JOKER_ANIM_DAMAGE].joker.maxFrame;
        if (maxFrame < 1) maxFrame = 1;

        if (IsFighterAtGroundLevel(position)) {
            currentFrame = maxFrame - 1;
            damageGroundHold += animSteps;
            if (damageGroundHold >= JOKER_DAMAGE_GROUND_HOLD_TICKS) {
                BeginRecover();
            }
        }
        else if (currentFrame < maxFrame - 1) {
            AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, JOKER_DAMAGE_ANIM_TICKS, maxFrame);
            damageGroundHold = 0;
        }
        else {
            currentFrame = maxFrame - 1;
            damageGroundHold = 0;
        }

        UpdateHurtbox();
        return;
    }

    if (currentState == JOKER_RECOVER) {
        if (!g_JokerAnims[JOKER_ANIM_RECOVER].joker.texture) {
            FinishRecoverToStance();
            return;
        }

        maxFrame = g_JokerAnims[JOKER_ANIM_RECOVER].joker.maxFrame;
        if (maxFrame < 1) {
            FinishRecoverToStance();
            return;
        }

        PinFighterToGround(position, verticalVelocity);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, JOKER_RECOVER_ANIM_TICKS, maxFrame)) {
            FinishRecoverToStance();
        }
        UpdateHurtbox();
        return;
    }

    if (isReturningToPosition || shouldReturnToOriginal) {
        position = originalPosition;
        isReturningToPosition = false;
        shouldReturnToOriginal = false;
        EnterStance();
        UpdateHurtbox();
        return;
    }

    ClampPosition();
    position = originalPosition;
    if (Fighter* opponent = GetOpponent(*this)) {
        facingDirection = (opponent->GetPosition().x >= position.x) ? 1 : -1;
    }
    else {
        facingDirection = -1;
    }

    if (currentState == JOKER_IDLE) {
        maxFrame = g_JokerAnims[JOKER_ANIM_IDLE].joker.maxFrame;
        if (maxFrame < 1 || !g_JokerAnims[JOKER_ANIM_IDLE].joker.texture) {
            EnterStance();
            UpdateHurtbox();
            return;
        }

        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, JOKER_IDLE_ANIM_TICKS, maxFrame)) {
            EnterStance();
        }
        UpdateHurtbox();
        return;
    }

    if (currentState == JOKER_STAND) {
        maxFrame = g_JokerAnims[JOKER_ANIM_STANCE].joker.maxFrame;
        if (maxFrame < 1) maxFrame = 1;

        AdvanceLoopFrame(animAccumulator, currentFrame, animSteps, MAKOTO_LOOP_TICKS_SLOW, maxFrame);

        idleWaitFrames += steps;
        if (idleWaitFrames >= IDLE_THRESHOLD_FRAMES) {
            EnterIdle();
        }
    }

    UpdateHurtbox();
}

void Joker::UpdateHuman(int steps) {
    Fighter* opponent = GetOpponent(*this);
    if (!opponent) return;

    const int animSteps = 1;
    const bool attackDownNow = IsGameMouseDown(VK_LBUTTON);
    const bool attackJustPressed = attackDownNow && !attackButtonHeld;
    attackButtonHeld = attackDownNow;

    const bool shiftHeld = IsGameKeyDown(DIK_LSHIFT) || IsGameKeyDown(DIK_RSHIFT);
    bool isRunning = shiftHeld;
    bool isMoving = false;
    float moveDirX = 0.0f;

    if (IsGameKeyDown(DIK_LEFT) || IsGameKeyDown(DIK_A)) {
        facingDirection = -1;
        moveDirX = -1.0f;
        isMoving = true;
    }
    if (IsGameKeyDown(DIK_RIGHT) || IsGameKeyDown(DIK_D)) {
        facingDirection = 1;
        moveDirX = 1.0f;
        isMoving = true;
    }

    if (isRunning && isMoving) {
        if (!DrainStaminaWhileRunning(animSteps)) {
            isRunning = false;
        }
    }
    else {
        RegenStamina(animSteps);
    }
    int currentVelocity = isRunning ? (velocity * 2) : velocity;

    const bool allowsMove =
        currentState == JOKER_STAND ||
        currentState == JOKER_WALK ||
        currentState == JOKER_RUN ||
        currentState == JOKER_GUARD ||
        currentState == JOKER_GUARD_AIR;

    if (allowsMove && isMoving && IsOnGround()) {
        TryApplyHorizontalDelta(moveDirX * currentVelocity * steps);
        ClampPosition();
    }

    if (currentState == JOKER_INTRO) {
        maxFrame = GetMaxFrameForState(JOKER_INTRO);
        if (maxFrame < 1 || !g_JokerAnims[JOKER_ANIM_INTRO].joker.texture) {
            EnterStance();
            UpdateHurtbox();
            return;
        }
        if (introDisplayHold > 0) {
            currentFrame = introLastFrame;
            introDisplayHold -= animSteps;
            if (introDisplayHold <= 0) {
                introDisplayHold = 0;
                EnterStance();
            }
            UpdateHurtbox();
            return;
        }
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, JOKER_INTRO_TICKS, maxFrame)) {
            introLastFrame = (maxFrame > 0) ? (maxFrame - 1) : 0;
            currentFrame = introLastFrame;
            introDisplayHold = INTRO_END_HOLD_FRAMES;
        }
        UpdateHurtbox();
        return;
    }

    if (currentState == JOKER_DAMAGE || isHit) {
        if (currentState != JOKER_DAMAGE) {
            BeginHitReaction(0.0f);
        }
        ApplyGravity(steps);
        PinFighterToGround(position, verticalVelocity);
        maxFrame = GetMaxFrameForState(JOKER_DAMAGE);
        if (maxFrame < 1) maxFrame = 1;

        if (IsFighterAtGroundLevel(position)) {
            currentFrame = maxFrame - 1;
            damageGroundHold += animSteps;
            if (damageGroundHold >= JOKER_DAMAGE_GROUND_HOLD_TICKS) {
                BeginRecover();
            }
        }
        else if (currentFrame < maxFrame - 1) {
            AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, JOKER_DAMAGE_ANIM_TICKS, maxFrame);
            damageGroundHold = 0;
        }
        else {
            currentFrame = maxFrame - 1;
            damageGroundHold = 0;
        }
        UpdateHurtbox();
        return;
    }

    if (currentState == JOKER_RECOVER) {
        maxFrame = GetMaxFrameForState(JOKER_RECOVER);
        if (!g_JokerAnims[JOKER_ANIM_RECOVER].joker.texture || maxFrame < 1) {
            FinishRecoverToStance();
            return;
        }
        PinFighterToGround(position, verticalVelocity);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, JOKER_RECOVER_ANIM_TICKS, maxFrame)) {
            FinishRecoverToStance();
        }
        UpdateHurtbox();
        return;
    }

    if (currentState == JOKER_TAUNT) {
        maxFrame = GetMaxFrameForState(JOKER_TAUNT);
        const JokerTexture& mona = g_JokerAnims[JOKER_ANIM_TAUNT].jokerEffect;
        const int monaMax = (mona.texture && mona.maxFrame > 0) ? mona.maxFrame : 1;

        bool jokerDone = false;
        if (currentFrame < maxFrame - 1) {
            jokerDone = AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, JOKER_TAUNT_TICKS, maxFrame);
        }
        else {
            jokerDone = true;
            currentFrame = maxFrame - 1;
        }

        // Mona keeps jumping through her full sheet even if Joker holds the last pose.
        skillEffectAccum += animSteps;
        while (skillEffectAccum >= JOKER_TAUNT_TICKS) {
            skillEffectAccum -= JOKER_TAUNT_TICKS;
            if (skillEffectFrame < monaMax - 1) {
                skillEffectFrame++;
            }
        }
        const bool monaDone = !mona.texture || skillEffectFrame >= monaMax - 1;
        if (jokerDone && monaDone) {
            EnterStance();
        }
        UpdateHurtbox();
        return;
    }

    if (currentState == JOKER_DODGE || currentState == JOKER_LEDGEROLL) {
        const float slide = (dodgeForward ? 1.0f : -1.0f) * (float)facingDirection;
        TryApplyHorizontalDelta(slide * (velocity * DODGE_SLIDE_SPEED) * steps);
        ClampPosition();
        maxFrame = GetMaxFrameForState(currentState);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, MAKOTO_ACTION_TICKS, maxFrame)) {
            EnterStance();
        }
        UpdateHurtbox();
        return;
    }

    if (currentState == JOKER_DASH) {
        TryApplyHorizontalDelta((float)facingDirection * (velocity * MAKOTO_DASH_SPEED_MULTIPLIER) * steps);
        ClampPosition();
        maxFrame = GetMaxFrameForState(JOKER_DASH);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, MAKOTO_ACTION_TICKS, maxFrame)) {
            hitThisAttack = false;
            EnterStance();
        }
        CheckAttackCollision(*opponent);
        UpdateHurtbox();
        return;
    }

    if (currentState == JOKER_JUMP) {
        const bool isLeftPressed = IsGameKeyDown(DIK_LEFT) || IsGameKeyDown(DIK_A);
        const bool isRightPressed = IsGameKeyDown(DIK_RIGHT) || IsGameKeyDown(DIK_D);
        const bool isEPressed = IsGameKeyDown(DIK_E);
        const bool isRPressed = IsGameKeyDown(DIK_R);
        const bool isSPressed = IsGameKeyDown(DIK_S);
        const bool isGuardPressed = IsHoldingGuardInput(*this);

        if (isLeftPressed) {
            facingDirection = -1;
            jumpHorizontalSpeed = -currentVelocity * FIGHTER_AIR_CONTROL_MULTIPLIER;
        }
        else if (isRightPressed) {
            facingDirection = 1;
            jumpHorizontalSpeed = currentVelocity * FIGHTER_AIR_CONTROL_MULTIPLIER;
        }

        if (isGuardPressed) {
            EnterActionState(JOKER_GUARD_AIR);
            UpdateHurtbox();
            return;
        }
        if (isEPressed && TryConsumeStamina(STAMINA_COST_ACTION)) {
            // Back air when moving opposite to facing.
            const bool movingBack =
                (facingDirection == 1 && moveDirX < 0.0f) ||
                (facingDirection == -1 && moveDirX > 0.0f);
            EnterActionState(movingBack ? JOKER_BACK_AIR : JOKER_FORWARD_AIR);
            UpdateHurtbox();
            return;
        }
        if (isRPressed && TryConsumeStamina(STAMINA_COST_ACTION)) {
            EnterActionState(JOKER_UP_AIR);
            UpdateHurtbox();
            return;
        }
        if (isSPressed && TryConsumeStamina(STAMINA_COST_ACTION)) {
            EnterActionState(JOKER_DOWN_AIR);
            UpdateHurtbox();
            return;
        }
        if (attackDownNow) {
            EnterActionState(JOKER_NEUTRAL_AIR);
            UpdateHurtbox();
            return;
        }

        TryApplyHorizontalDelta(jumpHorizontalSpeed * steps);
        ClampPosition();
        ApplyGravity(steps);

        maxFrame = GetMaxFrameForState(JOKER_JUMP);
        AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, MAKOTO_ACTION_TICKS, maxFrame);

        if (IsOnGround() && verticalVelocity >= 0.0f) {
            jumpCount = 0;
            EnterStance();
        }
        UpdateHurtbox();
        return;
    }

    if (IsAirAttackState(currentState) || currentState == JOKER_GUARD_AIR) {
        ApplyGravity(steps);
        ClampPosition();
        maxFrame = GetMaxFrameForState(currentState);

        if (currentState == JOKER_GUARD_AIR) {
            AdvanceLoopFrame(animAccumulator, currentFrame, animSteps, MAKOTO_LOOP_TICKS_SLOW, maxFrame);
            if (!IsHoldingGuardInput(*this)) {
                if (IsOnGround()) {
                    jumpCount = 0;
                    verticalVelocity = 0.0f;
                    position.y = CHARACTER_GROUND_Y;
                    EnterStance();
                }
                else {
                    currentState = JOKER_JUMP;
                    currentFrame = 0;
                    animAccumulator = 0;
                    maxFrame = GetMaxFrameForState(JOKER_JUMP);
                }
            }
            else if (IsOnGround()) {
                jumpCount = 0;
                verticalVelocity = 0.0f;
                position.y = CHARACTER_GROUND_Y;
                EnterActionState(JOKER_GUARD);
            }
            UpdateHurtbox();
            return;
        }

        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, MAKOTO_ACTION_TICKS, maxFrame) ||
            (IsOnGround() && verticalVelocity >= 0.0f)) {
            position.y = CHARACTER_GROUND_Y;
            verticalVelocity = 0.0f;
            jumpCount = 0;
            hitThisAttack = false;
            EnterStance();
        }
        else {
            CheckAttackCollision(*opponent);
        }
        UpdateHurtbox();
        return;
    }

    if (currentState == JOKER_ATTACK ||
        currentState == JOKER_FORWARD_ATTACK ||
        currentState == JOKER_UP_ATTACK ||
        currentState == JOKER_DOWN_ATTACK ||
        currentState == JOKER_FORWARD_SMASH ||
        currentState == JOKER_UP_SMASH ||
        currentState == JOKER_DOWN_SMASH ||
        currentState == JOKER_NEUTRAL_SPECIAL ||
        currentState == JOKER_NEUTRAL_AIR_SPECIAL ||
        currentState == JOKER_EIHA ||
        currentState == JOKER_EIGAON ||
        IsAllOutPhase(currentState) ||
        currentState == JOKER_PERSONA_SUMMON ||
        currentState == JOKER_PERSONA_RETURN) {
        maxFrame = GetMaxFrameForState(currentState);
        int ticks = MAKOTO_ACTION_TICKS;
        if (currentState == JOKER_ALL_OUT_MEMBER) {
            ticks = JOKER_ALL_OUT_MEMBER_TICKS;
        }
        else if (currentState == JOKER_ALL_OUT_FINISH) {
            ticks = JOKER_ALL_OUT_FINISH_TICKS;
            if (introDisplayHold > 0) {
                introDisplayHold -= animSteps;
                if (introDisplayHold <= 0) {
                    introDisplayHold = 0;
                    skillHit = false;
                    EnterStance();
                }
                UpdateHurtbox();
                return;
            }
        }
        else if (IsAllOutPhase(currentState)) {
            ticks = JOKER_ALL_OUT_TICKS;
            PullEnemyForUltimate(*this, *opponent, !skillHit && !opponent->IsHit());
        }
        else if (IsSkillState(currentState) || currentState == JOKER_EIHA || currentState == JOKER_EIGAON) {
            ticks = JOKER_SKILL_TICKS;
        }
        if (currentState == JOKER_NEUTRAL_AIR_SPECIAL) {
            ApplyGravity(steps);
        }
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, ticks, maxFrame)) {
            hitThisAttack = false;
            if (currentState == JOKER_ALL_OUT_ATTACK) {
                EnterActionState(JOKER_ALL_OUT_MEMBER);
            }
            else if (currentState == JOKER_ALL_OUT_MEMBER) {
                EnterActionState(JOKER_ALL_OUT_EFFECT);
            }
            else if (currentState == JOKER_ALL_OUT_EFFECT) {
                skillHit = false;
                EnterActionState(JOKER_ALL_OUT_FINISH);
            }
            else if (currentState == JOKER_ALL_OUT_FINISH) {
                // Hold the last finish pose so the outro is clearly visible.
                currentFrame = maxFrame - 1;
                introDisplayHold = JOKER_ALL_OUT_FINISH_HOLD_FRAMES;
            }
            else {
                skillHit = false;
                EnterStance();
            }
        }
        else if (IsMeleeAttackState(currentState)) {
            CheckAttackCollision(*opponent);
        }
        else if (IsSkillState(currentState)) {
            UpdateSkillHits(*opponent, animSteps);
        }
        UpdateHurtbox();
        return;
    }

    if (currentState == JOKER_IDLE) {
        const bool checkInput = isMoving ||
            IsGameKeyDown(DIK_SPACE) ||
            IsGameKeyDown(DIK_J) ||
            attackDownNow ||
            IsGameMouseDown(VK_RBUTTON) ||
            IsGameKeyDown(DIK_I) ||
            IsGameKeyDown(DIK_T) ||
            IsGameKeyDown(DIK_E) ||
            IsGameKeyDown(DIK_R) ||
            IsGameKeyDown(DIK_S) ||
            IsGameKeyDown(DIK_1) ||
            IsGameKeyDown(DIK_2) ||
            IsGameKeyDown(DIK_3) ||
            IsGameKeyDown(DIK_4) ||
            IsGameKeyDown(DIK_5);
        if (checkInput) {
            EnterStance();
            UpdateHurtbox();
            return;
        }
        maxFrame = GetMaxFrameForState(JOKER_IDLE);
        if (maxFrame < 1 || !g_JokerAnims[JOKER_ANIM_IDLE].joker.texture) {
            EnterStance();
            UpdateHurtbox();
            return;
        }
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, JOKER_IDLE_ANIM_TICKS, maxFrame)) {
            EnterStance();
        }
        UpdateHurtbox();
        return;
    }

    const bool isJumpPressed = IsGameKeyDown(DIK_SPACE);
    const bool isDashPressed = IsGameKeyDown(DIK_J);
    const bool isDodgePressed = IsGameMouseDown(VK_RBUTTON);
    const bool isGuardPressed = IsHoldingGuardInput(*this);
    const bool isTauntPressed = IsGameKeyDown(DIK_T);
    const bool isAttackPressed = attackJustPressed;
    const bool isSideAtkPressed = IsGameKeyDown(DIK_E);
    const bool isAtkUpPressed = IsGameKeyDown(DIK_R);
    const bool isDownPressed = IsGameKeyDown(DIK_S);
    const bool isEihaPressed = IsGameKeyDown(DIK_1);
    const bool isEigaonPressed = IsGameKeyDown(DIK_2);
    const bool isNeutralSpecialPressed = IsGameKeyDown(DIK_3);
    const bool isAllOutPressed = IsGameKeyDown(DIK_5);

    const bool hasAnyInput = isMoving || isJumpPressed || isDashPressed || attackDownNow ||
        isDodgePressed || isGuardPressed || isTauntPressed || isSideAtkPressed ||
        isAtkUpPressed || isDownPressed || isEihaPressed || isEigaonPressed ||
        isNeutralSpecialPressed || isAllOutPressed;

    if (hasAnyInput) idleWaitFrames = 0;
    else if (currentState == JOKER_STAND) idleWaitFrames += steps;

    int nextState = JOKER_STAND;

    if (isEihaPressed) {
        // Same flow as Eigaon: Joker eiha + Arsene eiha together (no persona! intro).
        nextState = JOKER_EIHA;
    }
    else if (isEigaonPressed) {
        nextState = JOKER_EIGAON;
    }
    else if (isNeutralSpecialPressed) {
        nextState = IsOnGround() ? JOKER_NEUTRAL_SPECIAL : JOKER_NEUTRAL_AIR_SPECIAL;
    }
    else if (isAllOutPressed) {
        // 5 → all-out_attack → member → effect(hit) → finish
        nextState = JOKER_ALL_OUT_ATTACK;
        skillEndHold = 0;
        if (Fighter* foe = GetOpponent(*this)) {
            PullEnemyForUltimate(*this, *foe, true);
        }
    }
    else if (isTauntPressed) {
        nextState = JOKER_TAUNT;
    }
    else if (isJumpPressed && IsOnGround()) {
        nextState = JOKER_JUMP;
        jumpCount = 1;
        verticalVelocity = FIGHTER_JUMP_VELOCITY;
        jumpHorizontalSpeed = (moveDirX != 0.0f) ? (moveDirX * currentVelocity * FIGHTER_AIR_CONTROL_MULTIPLIER) : 0.0f;
    }
    else if (isDashPressed && TryConsumeStamina(STAMINA_COST_ACTION)) {
        nextState = JOKER_DASH;
    }
    else if (isDodgePressed && TryConsumeStamina(STAMINA_COST_ACTION)) {
        nextState = (shiftHeld) ? JOKER_LEDGEROLL : JOKER_DODGE;
        const bool isLeftPressed = IsGameKeyDown(DIK_LEFT) || IsGameKeyDown(DIK_A);
        const bool isRightPressed = IsGameKeyDown(DIK_RIGHT) || IsGameKeyDown(DIK_D);
        if (isRightPressed && !isLeftPressed) dodgeForward = (facingDirection == 1);
        else if (isLeftPressed && !isRightPressed) dodgeForward = (facingDirection == -1);
        else dodgeForward = false;
    }
    else if (shiftHeld && isSideAtkPressed && TryConsumeStamina(STAMINA_COST_ACTION)) {
        nextState = JOKER_FORWARD_SMASH;
    }
    else if (shiftHeld && isAtkUpPressed && TryConsumeStamina(STAMINA_COST_ACTION)) {
        nextState = JOKER_UP_SMASH;
    }
    else if (shiftHeld && isDownPressed && isAttackPressed && TryConsumeStamina(STAMINA_COST_ACTION)) {
        nextState = JOKER_DOWN_SMASH;
    }
    else if (isDownPressed && isAttackPressed) {
        nextState = JOKER_DOWN_ATTACK;
    }
    else if (isSideAtkPressed) {
        nextState = JOKER_FORWARD_ATTACK;
    }
    else if (isAtkUpPressed) {
        nextState = JOKER_UP_ATTACK;
    }
    else if (isAttackPressed) {
        nextState = JOKER_ATTACK;
    }
    else if (isMoving) {
        nextState = isRunning ? JOKER_RUN : JOKER_WALK;
    }
    else if (isGuardPressed) {
        nextState = IsOnGround() ? JOKER_GUARD : JOKER_GUARD_AIR;
    }
    else if (idleWaitFrames >= IDLE_THRESHOLD_FRAMES) {
        nextState = JOKER_IDLE;
        idleWaitFrames = 0;
    }
    else {
        nextState = JOKER_STAND;
    }

    if (currentState != nextState) {
        if (nextState == JOKER_IDLE) {
            EnterIdle();
        }
        else {
            EnterActionState(nextState);
            if (nextState == JOKER_JUMP) {
                // Preserve jump impulse set above (EnterActionState clears frame only).
                jumpCount = 1;
                verticalVelocity = FIGHTER_JUMP_VELOCITY;
                jumpHorizontalSpeed = (moveDirX != 0.0f)
                    ? (moveDirX * currentVelocity * FIGHTER_AIR_CONTROL_MULTIPLIER)
                    : 0.0f;
            }
            if (nextState == JOKER_DODGE) {
                const bool isLeftPressed = IsGameKeyDown(DIK_LEFT) || IsGameKeyDown(DIK_A);
                const bool isRightPressed = IsGameKeyDown(DIK_RIGHT) || IsGameKeyDown(DIK_D);
                if (isRightPressed && !isLeftPressed) dodgeForward = (facingDirection == 1);
                else if (isLeftPressed && !isRightPressed) dodgeForward = (facingDirection == -1);
                else dodgeForward = false;
            }
        }
    }

    maxFrame = GetMaxFrameForState(currentState);
    switch (currentState) {
    case JOKER_WALK:
    case JOKER_RUN:
        AdvanceLoopFrame(animAccumulator, currentFrame, animSteps, MAKOTO_LOOP_TICKS_FAST, maxFrame);
        break;
    case JOKER_GUARD:
    case JOKER_GUARD_AIR:
    case JOKER_STAND:
        AdvanceLoopFrame(animAccumulator, currentFrame, animSteps, MAKOTO_LOOP_TICKS_SLOW, maxFrame);
        break;
    default:
        break;
    }

    UpdateHurtbox();
}

void Joker::Update() {
    if (IsPlayingResultPose()) {
        maxFrame = GetMaxFrameForState(currentState);
        if (maxFrame < 1) maxFrame = 1;

        int steps = g_GameTimer.GetLastFramesToUpdate();
        if (steps <= 0) steps = 1;
        if (steps > GAME_TIMER_MAX_STEPS_PER_FRAME) steps = GAME_TIMER_MAX_STEPS_PER_FRAME;

        if (currentState == JOKER_WIN) {
            if (!resultPoseAnimLocked) {
                if (currentFrame < maxFrame - 1) {
                    if (AdvanceOneShotFrame(
                        animAccumulator,
                        currentFrame,
                        steps,
                        BATTLE_WIN_ANIM_TICKS,
                        maxFrame)) {
                        resultPoseAnimLocked = true;
                    }
                }
                else {
                    resultPoseAnimLocked = true;
                }
            }

            if (resultPoseAnimLocked) {
                if (resultPoseHoldFrame < 0) {
                    const JokerTexture& winTex = g_JokerAnims[JOKER_ANIM_WIN].joker;
                    if (winTex.texture) {
                        resultPoseHoldFrame = FindLastVisibleSheetFrame(
                            winTex.texture,
                            kJokerCellSize,
                            kJokerCellSize,
                            winTex.cols,
                            maxFrame);
                    }
                    else {
                        resultPoseHoldFrame = maxFrame - 1;
                    }
                }
                currentFrame = resultPoseHoldFrame;
            }
        }
        else if (currentState == JOKER_LOSE) {
            currentFrame = maxFrame - 1;
        }

        position.y = CHARACTER_GROUND_Y;
        verticalVelocity = 0.0f;
        UpdateHurtbox();
        return;
    }

    if (isDead) return;

    int steps = g_GameTimer.GetLastFramesToUpdate();
    if (steps <= 0) return;
    if (steps > GAME_TIMER_MAX_STEPS_PER_FRAME) steps = GAME_TIMER_MAX_STEPS_PER_FRAME;

    if (!IsHumanControlled() && IsTutorialBattleMode()) {
        UpdateSandbag(steps);
        return;
    }

    if (!IsHumanControlled() && IsCpuLockedInReaction(*this, currentState)) {
        UpdateSandbag(steps);
        return;
    }

    std::optional<AiInputScope> aiInputScope;
    if (!IsHumanControlled()) {
        aiInputScope.emplace();
        DriveSimpleAi(*this);
    }
    UpdateHuman(steps);
}

void Joker::TakeDamage(int damage) {
    if (isDead) return;

    int appliedDamage = damage;
    if (TryProcessGuardBlock(*this, damage, appliedDamage)) {
        if (appliedDamage > 0) {
            health -= appliedDamage;
            if (health < 0) health = 0;
        }
        trainingIdleFrames = 0;
        idleWaitFrames = 0;
        UpdateHurtbox();
        return;
    }

    // Always apply HP so follow-up hits / skills still hurt after knock-down.
    health -= appliedDamage;
    if (health < 0) health = 0;
    trainingIdleFrames = 0;
    idleWaitFrames = 0;

    if (isHit || currentState == JOKER_DAMAGE || currentState == JOKER_RECOVER) {
        if (!TRAINING_MODE && health <= 0) isDead = true;
        return;
    }

    float knockback = 0.0f;
    if (IsHumanControlled()) {
        knockback = (facingDirection == 1) ? -OPPONENT_MELEE_KNOCKBACK : OPPONENT_MELEE_KNOCKBACK;
    }
    BeginHitReaction(knockback);

    if (!TRAINING_MODE && health <= 0) {
        isDead = true;
    }
}

bool Joker::IsInGuardState() const {
    return currentState == JOKER_GUARD || currentState == JOKER_GUARD_AIR;
}

void Joker::HoldGuardState(bool airborne) {
    const int target = airborne ? JOKER_GUARD_AIR : JOKER_GUARD;
    if (currentState == target) {
        return;
    }
    EnterActionState(target);
}

void Joker::ApplySkillDamage(int damage) {
    if (isDead) return;

    health -= damage;
    if (health < 0) health = 0;
    trainingIdleFrames = 0;
    idleWaitFrames = 0;

    if (isHit || currentState == JOKER_DAMAGE || currentState == JOKER_RECOVER) {
        if (!TRAINING_MODE && health <= 0) isDead = true;
        return;
    }

    float knockback = 0.0f;
    if (IsHumanControlled()) {
        knockback = (facingDirection == 1) ? -OPPONENT_SKILL_KNOCKBACK : OPPONENT_SKILL_KNOCKBACK;
    }
    BeginHitReaction(knockback);

    if (!TRAINING_MODE && health <= 0) {
        isDead = true;
    }
}

bool Joker::IsSuperMoveActive() const {
    return IsAllOutPhase(currentState) ||
        currentState == JOKER_EIHA ||
        currentState == JOKER_EIGAON ||
        currentState == JOKER_NEUTRAL_SPECIAL ||
        currentState == JOKER_NEUTRAL_AIR_SPECIAL ||
        currentState == JOKER_PERSONA_SUMMON ||
        currentState == JOKER_PERSONA_RETURN;
}

D3DCOLOR Joker::GetOverlayColor() const {
    if (!IsSuperMoveActive()) return 0;
    return D3DCOLOR_ARGB(220, 0, 0, 0);
}

void Joker::BeginVictoryPose() {
    isHit = false;
    isStunned = false;
    isDamageAnimating = false;
    resultPoseAnimLocked = false;
    resultPoseHoldFrame = -1;
    EnterActionState(JOKER_WIN);
    position.y = CHARACTER_GROUND_Y;
    verticalVelocity = 0.0f;
    UpdateHurtbox();
}

void Joker::BeginDefeatPose() {
    isHit = false;
    isStunned = false;
    isDamageAnimating = false;
    resultPoseHoldFrame = -1;
    EnterActionState(JOKER_LOSE);
    if (maxFrame < 1) maxFrame = 1;
    currentFrame = maxFrame - 1;
    position.y = CHARACTER_GROUND_Y;
    verticalVelocity = 0.0f;
    UpdateHurtbox();
}

bool Joker::IsPlayingResultPose() const {
    return currentState == JOKER_WIN || currentState == JOKER_LOSE;
}

bool Joker::IsInCombatAction() const {
    if (IsMeleeAttackState(currentState) || IsSkillState(currentState) || IsAllOutPhase(currentState)) {
        return true;
    }
    switch (currentState) {
    case JOKER_JUMP: case JOKER_DASH: case JOKER_DODGE: case JOKER_LEDGEROLL:
    case JOKER_PERSONA_SUMMON: case JOKER_PERSONA_RETURN:
        return true;
    default:
        return false;
    }
}

void Joker::Reset() {
    ApplySlotSpawnDefaults();
    originalPosition = position;
    health = maxHealth;
    sp = 0;
    RefillStamina();
    isDead = false;
    resultPoseAnimLocked = false;
    isActive = true;
    trainingIdleFrames = 0;
    idleWaitFrames = 0;
    jumpCount = 0;
    jumpHorizontalSpeed = 0.0f;
    verticalVelocity = 0.0f;
    hitThisAttack = false;
    attackButtonHeld = false;
    ResetAllStates();
    UpdateHurtbox();
}

static JokerAnimId GetAnimForState(int state) {
    switch (state) {
    case JOKER_INTRO: return JOKER_ANIM_INTRO;
    case JOKER_IDLE: return JOKER_ANIM_IDLE;
    case JOKER_WALK: return JOKER_ANIM_WALK;
    case JOKER_RUN: return JOKER_ANIM_RUN;
    case JOKER_DASH: return JOKER_ANIM_DASH;
    case JOKER_JUMP: return JOKER_ANIM_JUMP;
    case JOKER_GUARD: return JOKER_ANIM_GUARD;
    case JOKER_GUARD_AIR: return JOKER_ANIM_GUARD_AIR;
    case JOKER_ATTACK: return JOKER_ANIM_ATTACK_COMBO;
    case JOKER_FORWARD_ATTACK: return JOKER_ANIM_FORWARD_ATTACK;
    case JOKER_UP_ATTACK: return JOKER_ANIM_UP_ATTACK;
    case JOKER_DOWN_ATTACK: return JOKER_ANIM_DOWN_ATTACK;
    case JOKER_FORWARD_SMASH: return JOKER_ANIM_FORWARD_SMASH;
    case JOKER_UP_SMASH: return JOKER_ANIM_UP_SMASH;
    case JOKER_DOWN_SMASH: return JOKER_ANIM_DOWN_SMASH;
    case JOKER_NEUTRAL_AIR: return JOKER_ANIM_NEUTRAL_AIR;
    case JOKER_FORWARD_AIR: return JOKER_ANIM_FORWARD_AIR;
    case JOKER_BACK_AIR: return JOKER_ANIM_BACK_AIR;
    case JOKER_DOWN_AIR: return JOKER_ANIM_DOWN_AIR;
    case JOKER_UP_AIR: return JOKER_ANIM_UP_AIR;
    case JOKER_NEUTRAL_SPECIAL: return JOKER_ANIM_NEUTRAL_SPECIAL;
    case JOKER_NEUTRAL_AIR_SPECIAL: return JOKER_ANIM_NEUTRAL_AIR_SPECIAL;
    case JOKER_EIHA: return JOKER_ANIM_EIHA;
    case JOKER_EIGAON: return JOKER_ANIM_EIGAON;
    case JOKER_ALL_OUT_ATTACK: return JOKER_ANIM_ALL_OUT_ATTACK;
    case JOKER_ALL_OUT_MEMBER: return JOKER_ANIM_ALL_OUT_MEMBER;
    case JOKER_ALL_OUT_EFFECT: return JOKER_ANIM_ALL_OUT_EFFECT;
    case JOKER_ALL_OUT_FINISH: return JOKER_ANIM_ALL_OUT_FINISH;
    case JOKER_PERSONA_SUMMON: return JOKER_ANIM_PERSONA_SUMMON;
    case JOKER_PERSONA_RETURN: return JOKER_ANIM_PERSONA_RETURN;
    case JOKER_DODGE: return JOKER_ANIM_DODGE;
    case JOKER_LEDGEROLL: return JOKER_ANIM_LEDGEROLL;
    case JOKER_TAUNT: return JOKER_ANIM_TAUNT;
    case JOKER_DAMAGE: return JOKER_ANIM_DAMAGE;
    case JOKER_RECOVER: return JOKER_ANIM_RECOVER;
    case JOKER_WIN: return JOKER_ANIM_WIN;
    case JOKER_LOSE: return JOKER_ANIM_LOSE;
    case JOKER_STAND:
    default: return JOKER_ANIM_STANCE;
    }
}

void Joker::DrawArseneSprite(LPD3DXSPRITE sprite, JokerTexture& tex, int frame, D3DCOLOR color) const {
    D3DXVECTOR3 arsenePos(
        position.x - (float)facingDirection * ARSENE_BEHIND_HORIZONTAL,
        position.y - ARSENE_BEHIND_VERTICAL,
        0.0f);
    // Match Eigaon pairing: clamp to Arsene sheet length (do not wrap past last pose).
    int arseneFrame = frame;
    if (tex.maxFrame > 0) {
        if (arseneFrame < 0) arseneFrame = 0;
        if (arseneFrame >= tex.maxFrame) arseneFrame = tex.maxFrame - 1;
    }
    DrawJokerLayerSprite(sprite, tex, arseneFrame, arsenePos, facingDirection,
        ARSENE_BODY_HEIGHT, ARSENE_FEET_Y, color);
}

// Damage sheet feet Y measured from each cell (knockdown art sits much higher in the cell).
static float GetJokerDamageFeetY(int frameIndex) {
    return SampleFeetYTable(JOKER_DAMAGE_FEET_Y, (int)(sizeof(JOKER_DAMAGE_FEET_Y) / sizeof(JOKER_DAMAGE_FEET_Y[0])), frameIndex);
}

void Joker::DrawBodySprite(LPD3DXSPRITE sprite, JokerTexture& tex, int frame, const D3DXVECTOR3& pos, D3DCOLOR color) const {
    float bodyHeight = JOKER_BODY_HEIGHT;
    float feetY = JOKER_FEET_Y;
    if (currentState == JOKER_DAMAGE || currentState == JOKER_RECOVER) {
        feetY = MeasureTextureFrameBottomY(
            tex.texture,
            frame,
            kJokerCellSize,
            kJokerCellSize,
            tex.cols);
    }
    else if (currentState == JOKER_ALL_OUT_MEMBER) {
        // Same scale as normal Joker; high feet so lower portraits aren't clipped underground.
        feetY = JOKER_ALL_OUT_MEMBER_FEET_Y;
    }
    DrawJokerLayerSprite(sprite, tex, frame, pos, facingDirection, bodyHeight, feetY, color);
}

void Joker::DrawEffectSprite(LPD3DXSPRITE sprite, JokerTexture& tex, int frame, const D3DXVECTOR3& pos, float bodyHeight, float feetY, D3DCOLOR color) const {
    DrawJokerLayerSprite(sprite, tex, frame, pos, facingDirection, bodyHeight, feetY, color);
}

void Joker::DrawSkillEffectOnOpponent(LPD3DXSPRITE sprite, JokerTexture& tex, int frame, D3DCOLOR color) const {
    if (!tex.texture) return;
    // Same as Makoto AGI/Mabufu: feet-anchor the effect cell on the enemy hurtbox center.
    RECT rect;
    SetJokerFrameRect(rect, tex, frame);
    DrawScaledCharacterSprite(
        sprite,
        tex.texture,
        &rect,
        skillEffectPos,
        facingDirection,
        AGI_EFFECT_SCALE * PERSONA_EFFECT_SCALE,
        color,
        (float)kJokerCellSize,
        JOKER_FEET_Y);
}

void Joker::Render(LPD3DXSPRITE sprite) {
    if (isDead && !IsPlayingResultPose() && !IsBattleEndSequence()) return;

    UpdateHurtbox();

    JokerAnimId animId = GetAnimForState(currentState);
    const JokerTextureSet* setPtr = &g_JokerAnims[animId];
    // Fallback to stance if the current anim sheet is missing (prevents idle "vanishing").
    if (!setPtr->joker.texture) {
        animId = JOKER_ANIM_STANCE;
        setPtr = &g_JokerAnims[animId];
        if (!setPtr->joker.texture) return;
    }
    const JokerTextureSet& set = *setPtr;

    // Red tint only during the hit/damage reaction (not recover get-up).
    const bool showDamageTint =
        currentState == JOKER_DAMAGE ||
        isHit;
    const D3DCOLOR baseColor = showDamageTint
        ? D3DCOLOR_XRGB(255, 100, 100)
        : D3DCOLOR_XRGB(255, 255, 255);
    D3DCOLOR color = ApplySpriteTint(baseColor, GetSpriteTint());
    int bodyFrame = currentFrame;
    if (set.joker.maxFrame > 0) {
        if (bodyFrame < 0) bodyFrame = 0;
        if (bodyFrame >= set.joker.maxFrame) bodyFrame = set.joker.maxFrame - 1;
    }

    // Keep Arsene on stance/walk after combos — hiding on STAND caused a flash disappear.
    const bool hideArsene =
        currentState == JOKER_DAMAGE ||
        currentState == JOKER_RECOVER ||
        currentState == JOKER_INTRO ||
        currentState == JOKER_WIN ||
        currentState == JOKER_LOSE;

    D3DXVECTOR3 bodyPos = position;
    if (currentState == JOKER_ALL_OUT_MEMBER) {
        bodyPos.y -= JOKER_ALL_OUT_MEMBER_LIFT_Y;
    }
    else if (currentState == JOKER_ALL_OUT_FINISH) {
        bodyPos.y -= JOKER_ALL_OUT_FINISH_LIFT_Y;
    }
    else if (currentState == JOKER_ALL_OUT_ATTACK) {
        bodyPos.y -= JOKER_ALL_OUT_LIFT_Y;
    }

    // During all-out effect: keep Joker on stance while the effect sheet hits the opponent.
    if (currentState == JOKER_ALL_OUT_EFFECT) {
        const JokerTextureSet& stanceSet = g_JokerAnims[JOKER_ANIM_STANCE];
        if (stanceSet.pairedWithArsene && stanceSet.arsene.texture) {
            DrawArseneSprite(sprite, const_cast<JokerTexture&>(stanceSet.arsene), 0, color);
        }
        if (stanceSet.joker.texture) {
            DrawBodySprite(sprite, const_cast<JokerTexture&>(stanceSet.joker), 0, position, color);
        }
        Fighter* opponent = GetOpponent(*this);
        if (opponent && !opponent->isDead) {
            const AABB& hb = opponent->GetHurtbox();
            const_cast<Joker*>(this)->skillEffectPos =
                D3DXVECTOR3(hb.x + hb.width * 0.5f, hb.y + hb.height * 0.5f, 0.0f);
        }
        DrawSkillEffectOnOpponent(sprite, const_cast<JokerTexture&>(set.joker), bodyFrame, color);
        return;
    }

    // Eiha / Eigaon: Arsene behind, then Joker body (same pairing for both).
    if (set.pairedWithArsene && set.arsene.texture && !hideArsene) {
        DrawArseneSprite(sprite, const_cast<JokerTexture&>(set.arsene), bodyFrame, color);
    }

    DrawBodySprite(sprite, const_cast<JokerTexture&>(set.joker), bodyFrame, bodyPos, color);

    // Taunt / win: Mona on the ground in front of Joker (same slot as taunt).
    if (currentState == JOKER_TAUNT || currentState == JOKER_WIN) {
        const JokerTexture& monaTex = g_MonaWin.texture
            ? g_MonaWin
            : g_JokerAnims[JOKER_ANIM_TAUNT].jokerEffect;
        if (monaTex.texture) {
            int monaFrame = (currentState == JOKER_WIN) ? bodyFrame : skillEffectFrame;
            if (monaFrame < 0) monaFrame = 0;
            if (monaTex.maxFrame > 0 && monaFrame >= monaTex.maxFrame) {
                monaFrame = monaTex.maxFrame - 1;
            }
            const D3DXVECTOR3 monaPos(
                position.x + (float)facingDirection * MONA_TAUNT_OFFSET_X,
                position.y,
                0.0f);
            DrawJokerLayerSprite(
                sprite,
                const_cast<JokerTexture&>(monaTex),
                monaFrame,
                monaPos,
                facingDirection,
                JOKER_BODY_HEIGHT,
                MONA_TAUNT_FEET_Y,
                color);
        }
    }

    if (IsSkillState(currentState)) {
        Fighter* opponent = GetOpponent(*this);
        if (opponent && !opponent->isDead) {
            const AABB& hb = opponent->GetHurtbox();
            const_cast<Joker*>(this)->skillEffectPos =
                D3DXVECTOR3(hb.x + hb.width * 0.5f, hb.y + hb.height * 0.5f, 0.0f);
        }
        if (set.jokerEffect.texture) {
            DrawSkillEffectOnOpponent(sprite, const_cast<JokerTexture&>(set.jokerEffect), skillEffectFrame, color);
        }
        if (set.arseneEffect.texture) {
            DrawSkillEffectOnOpponent(sprite, const_cast<JokerTexture&>(set.arseneEffect), skillEffectFrame, color);
        }
    }
    else if (currentState != JOKER_TAUNT && (set.jokerEffect.texture || set.arseneEffect.texture)) {
        Fighter* opponent = GetOpponent(*this);
        if (opponent && !opponent->isDead) {
            const AABB& hb = opponent->GetHurtbox();
            const_cast<Joker*>(this)->skillEffectPos =
                D3DXVECTOR3(hb.x + hb.width * 0.5f, hb.y + hb.height * 0.5f, 0.0f);
        }
        if (set.arseneEffect.texture) {
            DrawSkillEffectOnOpponent(sprite, const_cast<JokerTexture&>(set.arseneEffect), bodyFrame, color);
        }
        if (set.jokerEffect.texture) {
            DrawSkillEffectOnOpponent(sprite, const_cast<JokerTexture&>(set.jokerEffect), bodyFrame, color);
        }
    }
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

        // 0 means Arsene uses the same frameCount as Joker.
        const int arseneFrames =
            (info.arseneFrameCount > 0) ? info.arseneFrameCount : info.frameCount;

        if (info.jokerFile && !LoadJokerSheet(set.joker, info.jokerFile, info.frameCount)) {
            // Stance is required; other sheets are optional so one missing file
            // does not wipe out idle/damage/etc.
            if (i == JOKER_ANIM_STANCE) {
                return false;
            }
        }
        if (info.arseneFile) {
            LoadJokerSheet(set.arsene, info.arseneFile, arseneFrames);
        }
        if (info.jokerEffectFile) {
            const int effectFrames = (i == JOKER_ANIM_TAUNT)
                ? MONA_TAUNT_FRAME_COUNT
                : info.frameCount;
            LoadJokerSheet(set.jokerEffect, info.jokerEffectFile, effectFrames);
        }
        if (info.arseneEffectFile) {
            LoadJokerSheet(set.arseneEffect, info.arseneEffectFile, arseneFrames);
        }
    }
    if (!LoadJokerSheet(g_MonaWin, "mona_win.png", MONA_TAUNT_FRAME_COUNT)) {
        g_MonaWin = g_JokerAnims[JOKER_ANIM_TAUNT].jokerEffect;
    }
    return g_JokerAnims[JOKER_ANIM_STANCE].joker.texture != nullptr;
}

void CleanUpJokerTextures() {
    if (g_MonaWin.texture &&
        g_MonaWin.texture != g_JokerAnims[JOKER_ANIM_TAUNT].jokerEffect.texture) {
        ReleaseJokerSheet(g_MonaWin);
    }
    g_MonaWin = JokerTexture{};
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
