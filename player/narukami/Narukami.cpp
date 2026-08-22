#include "Narukami.h"
#include "../../ai.h"
#include "../../config.h"
#include "../../renderer.h"
#include "../../game_logic.h"
#include "../../input.h"
#include <cmath>
#include <optional>
#include <stdio.h>

extern AttackData attackHitbox;
extern AttackData sideAttackHitbox;
extern AttackData attackUpHitbox;
extern AttackData downAttackHitbox;

namespace {

// Animation / combat timing (named locals — no magic numbers in logic).
// Match Makoto intro pacing so the longer 13-frame sheet does not flash by.
constexpr int kIntroTicks = MAKOTO_INTRO_TICKS;
constexpr int kActionTicks = MAKOTO_ACTION_TICKS;
constexpr int kCrossSlashTicks = NARUKAMI_CROSS_SLASH_TICKS;   // slower than normal melee
constexpr int kMyriadSummonTicks = NARUKAMI_MYRIAD_SUMMON_TICKS; // slower Myriad Truths
constexpr int kLoopSlowTicks = MAKOTO_LOOP_TICKS_SLOW;
constexpr int kLoopFastTicks = MAKOTO_LOOP_TICKS_FAST;
constexpr int kIdlePlayTicks = MAKOTO_IDLE_PLAY_TICKS;
constexpr int kSummonTicks = MAKOTO_SUMMON_TICKS;
constexpr int kPersonaEffectTicks = PERSONA_EFFECT_ANIM_DELAY;
constexpr int kDamageAnimTicks = JOKER_DAMAGE_ANIM_TICKS;
constexpr int kRecoverAnimTicks = JOKER_RECOVER_ANIM_TICKS;
constexpr int kDamageGroundHoldTicks = JOKER_DAMAGE_GROUND_HOLD_TICKS;
constexpr int kIdleWaitFrames = JOKER_IDLE_WAIT_FRAMES;
constexpr int kHitStunFrames = NARUKAMI_HIT_STUN_FRAMES;

constexpr float kJumpVelocity = FIGHTER_JUMP_VELOCITY;
constexpr float kAirControlMultiplier = NARUKAMI_AIR_CONTROL_MULTIPLIER;
constexpr float kDashSpeedMultiplier = MAKOTO_DASH_SPEED_MULTIPLIER;

constexpr int kZioDamage = 40;
constexpr int kZiodyneDamage = 60;
constexpr int kRagingLionDamage = 55;
constexpr int kBigGambleDamage = 70;
constexpr int kMyriadTruthsDamage = 95;
constexpr int kPersonaAttackDamage = 50;
constexpr int kSkillHitStartFrame = 1;
// Keys 1–5 consume SP.
constexpr int kSpCostZio = SP_COST_SUMMON_1;
constexpr int kSpCostZiodyne = SP_COST_SUMMON_2;
constexpr int kSpCostPersonaGround = SP_COST_THANATOS_SLASH;
constexpr int kSpCostPersonaAir = SP_COST_SUMMON_AIR;
constexpr int kSpCostMyriad = SP_COST_SUMMON_AIR_2;

constexpr float kIzanagiBehindX = PERSONA_BEHIND_HORIZONTAL * 0.65f;
constexpr float kIzanagiBehindY = ORPHEUS_BEHIND_VERTICAL * 0.55f;
constexpr float kPersonaDrawScale = NARUKAMI_IZANAGI_SCALE;
constexpr int kIntroEndHoldFrames = INTRO_END_HOLD_FRAMES;

struct NarukamiTexture {
    LPDIRECT3DTEXTURE9 texture = nullptr;
    int cols = 1;
    int rows = 1;
    int maxFrame = 1;
};

NarukamiTexture g_Stance;
NarukamiTexture g_Walk;
NarukamiTexture g_Run;
NarukamiTexture g_Dash;
NarukamiTexture g_Jump;
NarukamiTexture g_Crouch;
NarukamiTexture g_Guard;
NarukamiTexture g_GuardAir;
NarukamiTexture g_Attack;
NarukamiTexture g_SideAttack;
NarukamiTexture g_UpAttack;
NarukamiTexture g_DownAttack;
NarukamiTexture g_AirAttack;
NarukamiTexture g_Damage;
NarukamiTexture g_Recover;
NarukamiTexture g_Win;
NarukamiTexture g_IzanagiWin;
NarukamiTexture g_Intro;
NarukamiTexture g_IntroEffect;
NarukamiTexture g_Taunt;
NarukamiTexture g_NarukamiZio;
NarukamiTexture g_NarukamiZiodyne;
NarukamiTexture g_IzanagiZio;
NarukamiTexture g_IzanagiZiodyne;
NarukamiTexture g_ZioEffect;
NarukamiTexture g_ZiodyneEffect;
NarukamiTexture g_IzanagiAttack;
NarukamiTexture g_IzanagiAirAttack;
NarukamiTexture g_IzanagiSwiftStrike;
NarukamiTexture g_IzanagiCrossSlash;
NarukamiTexture g_PersonaAttack;
NarukamiTexture g_PersonaAirAttack;
NarukamiTexture g_RagingLion;
NarukamiTexture g_BigGamble;
NarukamiTexture g_MyriadTruths;
NarukamiTexture g_IzanagiMyriadTruths;

RECT g_SrcRect;

bool AdvanceOneShotFrame(int& accumulator, int& frame, int steps, int ticksPerFrame, int maxFrame) {
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

void AdvanceLoopFrame(int& accumulator, int& frame, int steps, int ticksPerFrame, int frameCount) {
    if (frameCount <= 0) return;
    accumulator += steps;
    while (accumulator >= ticksPerFrame) {
        accumulator -= ticksPerFrame;
        frame = (frame + 1) % frameCount;
    }
}

int ClampFrameIndex(const NarukamiTexture& tex, int frameIndex) {
    if (tex.maxFrame <= 0) return 0;
    if (frameIndex < 0) return 0;
    if (frameIndex >= tex.maxFrame) return tex.maxFrame - 1;
    return frameIndex;
}

// Source RECT from grid sheet: left/top = cellSize * (frame % cols, frame / cols).
void SetFrameRect(RECT& rect, const NarukamiTexture& tex, int frameIndex) {
    const int frame = ClampFrameIndex(tex, frameIndex);
    rect.left = NARUKAMI_CELL_SIZE * (frame % tex.cols);
    rect.top = NARUKAMI_CELL_SIZE * (frame / tex.cols);
    rect.right = rect.left + NARUKAMI_CELL_SIZE;
    rect.bottom = rect.top + NARUKAMI_CELL_SIZE;
}

static int GetSkillFacingDirection(const D3DXVECTOR3& selfPos, const D3DXVECTOR3& enemyCenter) {
    return (enemyCenter.x >= selfPos.x) ? 1 : -1;
}

static D3DXVECTOR3 GetNarukamiFrontIzanagiPos(const D3DXVECTOR3& narukamiPos, int facingDirection) {
    return D3DXVECTOR3(
        narukamiPos.x + (float)facingDirection * NARUKAMI_IZANAGI_SPRAY_FORWARD,
        narukamiPos.y - NARUKAMI_IZANAGI_SPRAY_VERTICAL,
        0.0f);
}

static D3DXVECTOR3 GetNarukamiZioHitPos(const Fighter& enemy) {
    const AABB& hb = enemy.GetHurtbox();
    return D3DXVECTOR3(
        hb.x + hb.width * 0.5f,
        hb.y + hb.height * NARUKAMI_ZIO_HIT_BODY_Y_RATIO,
        0.0f);
}

static D3DXVECTOR3 GetNarukamiMyriadIzanagiPos(const D3DXVECTOR3& narukamiPos) {
    const float bodyScreenH = (float)SCREEN_HEIGHT * MAKOTO_SCREEN_HEIGHT_RATIO;
    return D3DXVECTOR3(
        narukamiPos.x,
        narukamiPos.y - bodyScreenH - NARUKAMI_MYRIAD_IZANAGI_HEAD_GAP,
        0.0f);
}

static D3DXVECTOR3 GetNarukamiZiodynePos(
    const D3DXVECTOR3& enemyCenter,
    int skillFacing) {
    return D3DXVECTOR3(
        enemyCenter.x - (float)skillFacing * NARUKAMI_ZIODYNE_HORIZONTAL_OFFSET,
        enemyCenter.y + NARUKAMI_ZIODYNE_VERTICAL_OFFSET,
        0.0f);
}

static void DrawNarukamiZiodyneEffect(
    LPD3DXSPRITE sprite,
    const D3DXVECTOR3& pos,
    int skillFacing,
    int frame,
    D3DCOLOR color)
{
    const NarukamiTexture& effectTex = g_ZiodyneEffect;
    if (!sprite || !effectTex.texture) return;

    SetFrameRect(g_SrcRect, effectTex, frame);
    DrawScaledCharacterSprite(
        sprite,
        effectTex.texture,
        &g_SrcRect,
        pos,
        skillFacing,
        PERSONA_EFFECT_SCALE,
        color,
        (float)MAKOTO_CELL_SIZE,
        MAKOTO_FEET_Y);
}

const NarukamiTexture* GetTextureForState(int state) {
    switch (state) {
    case NARUKAMI_INTRO: return &g_Intro;
    case NARUKAMI_INTRO_DISCARD: return &g_IntroEffect;
    case NARUKAMI_STANCE:
    case NARUKAMI_IDLE: return &g_Stance;
    case NARUKAMI_WALK: return &g_Walk;
    case NARUKAMI_RUN: return &g_Run;
    case NARUKAMI_DASH: return &g_Dash;
    case NARUKAMI_JUMP: return &g_Jump;
    case NARUKAMI_PERSONA_SUMMON: return &g_PersonaAttack;
    case NARUKAMI_PERSONA_AIR_SUMMON: return &g_PersonaAirAttack;
    case NARUKAMI_ATTACK:
    case NARUKAMI_CROUCH_ATTACK: return &g_Attack;
    case NARUKAMI_CROUCH: return &g_Crouch;
    case NARUKAMI_GUARD: return &g_Guard;
    case NARUKAMI_GUARD_AIR:
        return g_GuardAir.texture ? &g_GuardAir : &g_Guard;
    case NARUKAMI_SIDE_ATTACK: return &g_SideAttack;
    case NARUKAMI_ATTACK_UP: return &g_UpAttack;
    case NARUKAMI_DOWN_ATTACK: return &g_DownAttack;
    case NARUKAMI_NEUTRAL_AIR: return &g_AirAttack;
    case NARUKAMI_DAMAGE: return &g_Damage;
    case NARUKAMI_RECOVER: return &g_Recover;
    case NARUKAMI_WIN: return &g_Win;
    case NARUKAMI_LOSE: return &g_Damage;
    case NARUKAMI_TAUNT: return &g_Taunt;
    case NARUKAMI_SUMMON_ZIO: return &g_NarukamiZio;
    case NARUKAMI_SUMMON_ZIODYNE: return &g_NarukamiZiodyne;
    case NARUKAMI_RAGING_LION: return &g_RagingLion;
    case NARUKAMI_BIG_GAMBLE: return &g_BigGamble;
    case NARUKAMI_MYRIAD_TRUTHS: return &g_MyriadTruths;
    default: return nullptr;
    }
}

int GetMaxFrameForState(int state) {
    const NarukamiTexture* tex = GetTextureForState(state);
    return tex ? tex->maxFrame : 1;
}

bool AllowsMovement(int state) {
    switch (state) {
    case NARUKAMI_STANCE:
    case NARUKAMI_WALK:
    case NARUKAMI_RUN:
    case NARUKAMI_CROUCH:
    case NARUKAMI_GUARD:
    case NARUKAMI_GUARD_AIR:
        return true;
    default:
        return false;
    }
}

bool ResetsAnimationOnEnter(int state) {
    switch (state) {
    case NARUKAMI_STANCE:
    case NARUKAMI_WALK:
    case NARUKAMI_RUN:
    case NARUKAMI_GUARD:
    case NARUKAMI_GUARD_AIR:
        return false;
    default:
        return true;
    }
}

bool IsMeleeState(int state) {
    switch (state) {
    case NARUKAMI_ATTACK:
    case NARUKAMI_CROUCH_ATTACK:
    case NARUKAMI_SIDE_ATTACK:
    case NARUKAMI_ATTACK_UP:
    case NARUKAMI_DOWN_ATTACK:
    case NARUKAMI_NEUTRAL_AIR:
    case NARUKAMI_DASH:
    case NARUKAMI_RAGING_LION:
    case NARUKAMI_BIG_GAMBLE:
        return true;
    default:
        return false;
    }
}

bool IsSummonState(int state) {
    return state == NARUKAMI_SUMMON_ZIO ||
        state == NARUKAMI_SUMMON_ZIODYNE ||
        state == NARUKAMI_MYRIAD_TRUTHS;
}

bool IsSpecialMeleeState(int state) {
    return state == NARUKAMI_RAGING_LION || state == NARUKAMI_BIG_GAMBLE;
}

void BuildAttackBox(
    const D3DXVECTOR3& fighterPos,
    int facingDirection,
    float scale,
    const AttackData& data,
    float& attackX,
    float& attackY,
    float& boxW,
    float& boxH)
{
    boxW = data.width * scale;
    boxH = data.height * scale;
    const float forward = data.offsetX * scale;
    const float vertical = data.offsetY * scale;

    if (facingDirection == 1) {
        attackX = fighterPos.x + forward;
    }
    else {
        attackX = fighterPos.x - forward - boxW;
    }
    attackY = fighterPos.y + vertical;
}

D3DXVECTOR3 GetEnemyHurtboxCenter(const Fighter& enemy) {
    const AABB& hb = enemy.GetHurtbox();
    return D3DXVECTOR3(hb.x + hb.width * 0.5f, hb.y + hb.height * 0.5f, 0.0f);
}

static D3DXVECTOR3 GetNarukamiMyriadRipplePos(const Fighter& enemy) {
    const D3DXVECTOR3 enemyCenter = GetEnemyHurtboxCenter(enemy);
    return D3DXVECTOR3(
        enemyCenter.x,
        enemyCenter.y - NARUKAMI_MYRIAD_RIPPLE_VERTICAL,
        0.0f);
}

float GetNarukamiBodyFeetY(int state, int frameIndex, bool groundedKnockdown = false) {
    if (state == NARUKAMI_WALK || state == NARUKAMI_RUN) {
        return NARUKAMI_RUN_FEET_Y;
    }
    if (state == NARUKAMI_CROUCH || state == NARUKAMI_CROUCH_ATTACK) {
        return NARUKAMI_CROUCH_FEET_Y;
    }
    if (state == NARUKAMI_DAMAGE || state == NARUKAMI_LOSE) {
        if (groundedKnockdown) {
            return GetGroundedDamageDrawFeetY(
                NARUKAMI_DAMAGE_FEET_Y,
                (int)(sizeof(NARUKAMI_DAMAGE_FEET_Y) / sizeof(NARUKAMI_DAMAGE_FEET_Y[0])),
                frameIndex,
                NARUKAMI_KNOCKDOWN_GROUND_FEET_Y);
        }
        return SampleFeetYTable(
            NARUKAMI_DAMAGE_FEET_Y,
            (int)(sizeof(NARUKAMI_DAMAGE_FEET_Y) / sizeof(NARUKAMI_DAMAGE_FEET_Y[0])),
            frameIndex);
    }
    if (state == NARUKAMI_RECOVER) {
        if (groundedKnockdown) {
            return GetGroundedRecoverDrawFeetY(
                NARUKAMI_RECOVER_FEET_Y_TABLE,
                (int)(sizeof(NARUKAMI_RECOVER_FEET_Y_TABLE) / sizeof(NARUKAMI_RECOVER_FEET_Y_TABLE[0])),
                frameIndex);
        }
        return SampleFeetYTable(
            NARUKAMI_RECOVER_FEET_Y_TABLE,
            (int)(sizeof(NARUKAMI_RECOVER_FEET_Y_TABLE) / sizeof(NARUKAMI_RECOVER_FEET_Y_TABLE[0])),
            frameIndex);
    }
    if (state == NARUKAMI_INTRO_DISCARD) {
        return NARUKAMI_STANCE_FEET_Y;
    }
    return NARUKAMI_STANCE_FEET_Y;
}

void DrawCenteredLayer(
    LPD3DXSPRITE sprite,
    const NarukamiTexture& tex,
    int frameIndex,
    const D3DXVECTOR3& centerPos,
    float scale,
    D3DCOLOR color)
{
    if (!sprite || !tex.texture) return;
    SetFrameRect(g_SrcRect, tex, frameIndex);
    DrawCenteredEffectSprite(
        sprite,
        tex.texture,
        &g_SrcRect,
        centerPos,
        scale,
        color,
        (float)NARUKAMI_CELL_SIZE);
}

static void DrawMyriadRippleWaves(
    LPD3DXSPRITE sprite,
    const D3DXVECTOR3& center,
    int rippleStep,
    D3DCOLOR /*tint*/)
{
    if (!sprite || rippleStep <= 0) return;

    const float progress = (float)rippleStep / (float)NARUKAMI_MYRIAD_RIPPLE_MAX_STEPS;

    for (int ring = 0; ring < 5; ++ring) {
        const float ringT = progress - (float)ring * 0.1f;
        if (ringT <= 0.0f || ringT > 1.0f) continue;

        const float fade = 1.0f - ringT;
        const int alpha = (int)(190.0f * fade);
        if (alpha <= 0) continue;

        const float radius =
            NARUKAMI_MYRIAD_RIPPLE_RING_START + ringT * NARUKAMI_MYRIAD_RIPPLE_RING_GROWTH * NARUKAMI_MYRIAD_RIPPLE_RING_SCALE;
        DrawDebugCircleRing(
            sprite,
            center.x,
            center.y,
            radius,
            D3DCOLOR_ARGB(alpha, 255, 255, 255),
            56);
    }
}

void DrawLayer(
    LPD3DXSPRITE sprite,
    const NarukamiTexture& tex,
    int frameIndex,
    const D3DXVECTOR3& pos,
    int facingDirection,
    float scale,
    D3DCOLOR color,
    float feetY)
{
    if (!sprite || !tex.texture) return;
    SetFrameRect(g_SrcRect, tex, frameIndex);
    DrawScaledCharacterSprite(
        sprite,
        tex.texture,
        &g_SrcRect,
        pos,
        facingDirection,
        scale,
        color,
        MAKOTO_BODY_HEIGHT,
        feetY);
}

bool LoadSheet(NarukamiTexture& tex, const char* path, int frameCount) {
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
        D3DCOLOR_XRGB(NARUKAMI_COLORKEY_R, NARUKAMI_COLORKEY_G, NARUKAMI_COLORKEY_B),
        NULL,
        NULL,
        &tex.texture);

    if (FAILED(hr) || !tex.texture) {
        return false;
    }

    ApplyNarukamiColorKey(tex.texture);

    D3DSURFACE_DESC desc;
    tex.texture->GetLevelDesc(0, &desc);
    tex.cols = (int)(desc.Width / NARUKAMI_CELL_SIZE);
    tex.rows = (int)(desc.Height / NARUKAMI_CELL_SIZE);
    if (tex.cols < 1) tex.cols = 1;
    if (tex.rows < 1) tex.rows = 1;

    const int gridFrames = tex.cols * tex.rows;
    tex.maxFrame = frameCount;
    if (tex.maxFrame <= 0 || tex.maxFrame > gridFrames) {
        tex.maxFrame = gridFrames;
    }
    if (tex.maxFrame < 1) tex.maxFrame = 1;
    return true;
}

void ReleaseSheet(NarukamiTexture& tex) {
    if (tex.texture) {
        tex.texture->Release();
        tex.texture = nullptr;
    }
    tex.cols = 1;
    tex.rows = 1;
    tex.maxFrame = 1;
}

} // namespace

Narukami::Narukami()
    : currentFrame(0)
    , maxFrame(1)
    , animAccumulator(0)
    , currentState(NARUKAMI_INTRO)
    , jumpCount(0)
    , jumpHorizontalSpeed(0.0f)
    , verticalVelocity(0.0f)
    , hitThisAttack(false)
    , dashHasHit(false)
    , attackButtonHeld(false)
    , skillHit(false)
    , jumpSpaceWasReleased(true)
    , personaAnimAccumulator(0)
    , izanagiFrame(0)
    , effectFrame(0)
    , showIzanagi(false)
    , showEffect(false)
    , izanagiPos(0.0f, 0.0f, 0.0f)
    , effectPos(0.0f, 0.0f, 0.0f)
    , discardPos(0.0f, 0.0f, 0.0f)
    , discardFlying(false)
    , noInputFrames(0)
    , idleWaitFrames(0)
    , damageGroundHold(0)
    , introDisplayHold(0)
    , introLastFrame(0)
    , introDiscardSteps(0)
    , pendingAttackState(0)
    , spawnPosition(0.0f, 0.0f, 0.0f)
{
    characterId = Char_Narukami;
    maxHealth = NARUKAMI_MAX_HEALTH;
    health = NARUKAMI_MAX_HEALTH;
    velocity = NARUKAMI_MOVE_SPEED;
    ApplySlotSpawnDefaults();
    spawnPosition = position;
    maxFrame = GetMaxFrameForState(NARUKAMI_INTRO);
    UpdateScaledHurtbox();
}

AABB Narukami::GetBodyCollisionBox() const {
    // Same body constants + anchor as Makoto; dash extends the live pushbox forward.
    AABB box = MakeLivePushbox(
        position,
        facingDirection,
        NARUKAMI_PUSHBOX_WIDTH,
        NARUKAMI_PUSHBOX_HEIGHT,
        GetMakotoDrawScale());

    if (currentState == NARUKAMI_DASH) {
        const float extend = DASH_HITBOX_FORWARD * GetMakotoDrawScale();
        if (facingDirection < 0) {
            box.x -= extend;
            box.width += extend;
        }
        else {
            box.width += extend;
        }
    }
    return box;
}

void Narukami::UpdateScaledHurtbox() {
    hurtbox = GetBodyCollisionBox();
}

bool Narukami::IsOnGround() const {
    return position.y >= CHARACTER_GROUND_Y - GROUND_CONTACT_EPSILON;
}

void Narukami::ApplyGravity(int steps) {
    // BMCS2224 Physics module: force → acceleration → velocity → position.
    ApplyPhysicsGravitySteps(steps, verticalVelocity);
}

bool Narukami::IsSuperMoveActive() const {
    return IsSummonState(currentState);
}

D3DCOLOR Narukami::GetOverlayColor() const {
    if (!IsSummonState(currentState)) return 0;
    // Full black stage is handled in Render(); keep a soft tint if needed elsewhere.
    return D3DCOLOR_ARGB(220, 0, 0, 0);
}

void Narukami::EnterState(int state) {
    if (ResetsAnimationOnEnter(state)) {
        currentFrame = 0;
        animAccumulator = 0;
        hitThisAttack = false;
    }
    currentState = state;
    maxFrame = GetMaxFrameForState(state);
    if (maxFrame < 1) maxFrame = 1;
}

void Narukami::CompleteToStance() {
    showIzanagi = false;
    showEffect = false;
    skillHit = false;
    personaAnimAccumulator = 0;
    izanagiFrame = 0;
    effectFrame = 0;
    hitThisAttack = false;
    dashHasHit = false;
    pendingAttackState = 0;
    jumpCount = 0;
    verticalVelocity = 0.0f;
    if (IsOnGround()) {
        position.y = CHARACTER_GROUND_Y;
    }
    EnterState(NARUKAMI_STANCE);
    idleWaitFrames = 0;
    noInputFrames = 0;
}

void Narukami::BeginSummon(int state) {
    EnterState(state);
    skillHit = false;
    skillEndHold = 0;
    showIzanagi = true;
    showEffect = true;
    izanagiFrame = 0;
    effectFrame = 0;
    personaAnimAccumulator = 0;
    izanagiPos = D3DXVECTOR3(
        position.x - (float)facingDirection * kIzanagiBehindX,
        position.y - kIzanagiBehindY,
        0.0f);
    if (Fighter* foe = GetOpponent(*this)) {
        PullEnemyForUltimate(*this, *foe, true);
    }
}

void Narukami::BeginIntroDiscard() {
    // After intro: sword hilt (intro_effect) flies behind and off-screen.
    currentState = NARUKAMI_INTRO_DISCARD;
    currentFrame = 0;
    animAccumulator = 0;
    introDisplayHold = 0;
    introDiscardSteps = 0;
    discardFlying = true;
    discardPos = position;
    maxFrame = GetMaxFrameForState(NARUKAMI_INTRO_DISCARD);
    if (maxFrame < 1) maxFrame = 1;
    if (!g_IntroEffect.texture) {
        discardFlying = false;
        CompleteToStance();
    }
}

void Narukami::BeginPersonaSummon(int summonState, int followUpAttack) {
    // Makoto-style: persona_attack / persona_air_attack first, then izanagi attack.
    pendingAttackState = followUpAttack;
    showIzanagi = false;
    showEffect = false;
    izanagiFrame = 0;
    EnterState(summonState);
}

void Narukami::BeginHitReaction() {
    isHit = true;
    hitStunTimer = kHitStunFrames;
    currentState = NARUKAMI_DAMAGE;
    currentFrame = 0;
    animAccumulator = 0;
    damageGroundHold = 0;
    maxFrame = GetMaxFrameForState(NARUKAMI_DAMAGE);
    if (maxFrame < 1) maxFrame = 1;
    showIzanagi = false;
    showEffect = false;
    skillHit = false;
    idleWaitFrames = 0;
    ApplyStandardHitReactionVertical(position, verticalVelocity, IsOnGround());
    if (IsOnGround()) {
        currentFrame = maxFrame - 1;
    }
}

void Narukami::BeginRecover() {
    isHit = false;
    hitStunTimer = 0;
    currentState = NARUKAMI_RECOVER;
    currentFrame = 0;
    animAccumulator = 0;
    maxFrame = GetMaxFrameForState(NARUKAMI_RECOVER);
    if (!g_Recover.texture || maxFrame < 1) {
        FinishRecover();
        return;
    }
}

void Narukami::FinishRecover() {
    // Stay at knockdown position so CPU AI can re-engage (no sandbag spawn snap).
    verticalVelocity = 0.0f;
    jumpCount = 0;
    isHit = false;
    hitStunTimer = 0;
    CompleteToStance();
    UpdateScaledHurtbox();
}

void Narukami::CheckAttackCollision(Fighter& enemy) {
    if (enemy.isDead) return;

    const float scale = GetCharacterRenderScale();
    AttackData* data = nullptr;

    int specialDamage = 0;
    switch (currentState) {
    case NARUKAMI_ATTACK:
    case NARUKAMI_CROUCH_ATTACK:
    case NARUKAMI_NEUTRAL_AIR:
        data = &attackHitbox;
        break;
    case NARUKAMI_SIDE_ATTACK:
        data = &sideAttackHitbox;
        break;
    case NARUKAMI_ATTACK_UP:
        data = &attackUpHitbox;
        break;
    case NARUKAMI_DOWN_ATTACK:
        data = &downAttackHitbox;
        break;
    case NARUKAMI_RAGING_LION:
        data = &sideAttackHitbox;
        specialDamage = kRagingLionDamage;
        break;
    case NARUKAMI_BIG_GAMBLE:
        data = &attackHitbox;
        specialDamage = kBigGambleDamage;
        break;
    case NARUKAMI_DASH:
        break;
    default:
        return;
    }

    if (currentState == NARUKAMI_DASH) {
        if (!dashHasHit &&
            currentFrame >= DASH_HIT_START_FRAME &&
            currentFrame <= DASH_HIT_END_FRAME) {
            const float dashW = DASH_HITBOX_WIDTH * scale;
            const float dashH = DASH_HITBOX_HEIGHT * scale;
            float dashX = 0.0f;
            if (facingDirection == 1) {
                dashX = position.x + DASH_HITBOX_FORWARD * scale;
            }
            else {
                dashX = position.x - DASH_HITBOX_FORWARD * scale - dashW;
            }
            const float dashY = position.y - DASH_HITBOX_UP * scale;
            const AABB dashBox = { dashX, dashY, dashW, dashH };
            if (CollisionHelper::AABBIntersect(dashBox, enemy.GetHurtbox())) {
                enemy.TakeDamage(DASH_HIT_DAMAGE);
                dashHasHit = true;
                RestoreSp(SP_GAIN_ON_HIT);
            }
        }
        return;
    }

    if (!data) return;
    if (currentFrame == 0) hitThisAttack = false;

    const int totalFrames = GetMaxFrameForState(currentState);
    const int startF = (data->startFrame > 0) ? data->startFrame : max(1, totalFrames / 4);
    const int endF = min(totalFrames - 1, data->endFrame);

    if (currentFrame >= startF && currentFrame <= endF) {
        float attackX = 0.0f;
        float attackY = 0.0f;
        float boxW = 0.0f;
        float boxH = 0.0f;
        BuildAttackBox(position, facingDirection, scale, *data, attackX, attackY, boxW, boxH);

        const AABB attackBox = { attackX, attackY, boxW, boxH };
        if (!hitThisAttack && CollisionHelper::AABBIntersect(attackBox, enemy.GetHurtbox())) {
            const int dmg = (specialDamage > 0) ? specialDamage : data->damage;
            enemy.TakeDamage(dmg);
            hitThisAttack = true;
            RestoreSp(SP_GAIN_ON_HIT);
        }
    }
}

void Narukami::UpdateSummon(int steps, Fighter& enemy) {
    const bool isZio = (currentState == NARUKAMI_SUMMON_ZIO);
    const bool isMyriad = (currentState == NARUKAMI_MYRIAD_TRUTHS);
    const bool isLaser = !isMyriad; // Zio / Ziodyne bolt
    const NarukamiTexture& bodyTex = isMyriad ? g_MyriadTruths : (isZio ? g_NarukamiZio : g_NarukamiZiodyne);
    const NarukamiTexture& izanagiTex = isMyriad ? g_IzanagiMyriadTruths : (isZio ? g_IzanagiZio : g_IzanagiZiodyne);
    const NarukamiTexture& effectTex = isZio ? g_ZioEffect : g_ZiodyneEffect;
    const int skillDamage = isMyriad ? kMyriadTruthsDamage : (isZio ? kZioDamage : kZiodyneDamage);

    // Force the foe in close for the skill framing (stop Y lock after they get hit so gravity works).
    PullEnemyForUltimate(*this, enemy, !skillHit && !enemy.IsHit());

    maxFrame = bodyTex.maxFrame;
    if (maxFrame < 1) maxFrame = 1;

    if (isMyriad) {
        izanagiPos = GetNarukamiMyriadIzanagiPos(position);
        effectPos = GetNarukamiMyriadRipplePos(enemy);
    }
    else if (isZio || isLaser) {
        const D3DXVECTOR3 enemyCenter = GetEnemyHurtboxCenter(enemy);
        const int skillFacing = GetSkillFacingDirection(position, enemyCenter);

        izanagiPos = GetNarukamiFrontIzanagiPos(position, facingDirection);

        if (isZio) {
            effectPos = GetNarukamiZioHitPos(enemy);
        }
        else {
            effectPos = GetNarukamiZiodynePos(enemyCenter, skillFacing);
        }
    }

    // Hold last body frame until persona / effect finishes (Makoto super style).
    bool bodyDone = false;
    const int bodyTicks = isMyriad
        ? kMyriadSummonTicks
        : NARUKAMI_ZIO_SUMMON_TICKS;
    const int personaTicks = isMyriad
        ? NARUKAMI_MYRIAD_PERSONA_TICKS
        : NARUKAMI_LASER_PERSONA_TICKS;
    if (currentFrame < maxFrame - 1) {
        bodyDone = AdvanceOneShotFrame(animAccumulator, currentFrame, steps, bodyTicks, maxFrame);
    }
    else {
        bodyDone = true;
        currentFrame = maxFrame - 1;
    }

    personaAnimAccumulator += steps;
    while (personaAnimAccumulator >= personaTicks) {
        personaAnimAccumulator -= personaTicks;
        if (showIzanagi && izanagiTex.maxFrame > 0 && izanagiFrame < izanagiTex.maxFrame - 1) {
            izanagiFrame++;
        }
        if (isLaser && showEffect && effectTex.maxFrame > 0 && effectFrame < effectTex.maxFrame - 1) {
            effectFrame++;
        }
    }

    const bool izanagiDone = !showIzanagi || izanagiTex.maxFrame < 1 ||
        izanagiFrame >= izanagiTex.maxFrame - 1;

    bool effectDone = true;
    if (isLaser && showEffect && effectTex.maxFrame > 0) {
        if (effectFrame < effectTex.maxFrame - 1) {
            effectDone = false;
            skillEndHold = 0;
        }
        else {
            skillEndHold += steps;
            effectDone = skillEndHold >= NARUKAMI_LASER_END_HOLD_STEPS;
        }
    }
    else if (isMyriad && showEffect) {
        effectFrame += steps;
        effectDone = effectFrame >= NARUKAMI_MYRIAD_RIPPLE_MAX_STEPS + NARUKAMI_MYRIAD_RIPPLE_END_HOLD_STEPS;
    }

    // Guaranteed skill hit (Makoto AGI style) — body or effect active frames.
    if (!skillHit) {
        const bool hitReady = isMyriad
            ? (izanagiFrame >= kSkillHitStartFrame || currentFrame >= max(1, maxFrame / 4))
            : (effectFrame >= kSkillHitStartFrame || currentFrame >= kSkillHitStartFrame);
        if (hitReady && !enemy.isDead) {
            enemy.ApplySkillDamage(skillDamage);
            skillHit = true;
        }
    }

    if (bodyDone && izanagiDone && effectDone) {
        CompleteToStance();
    }
}

void Narukami::UpdateSandbag(int steps) {
    const int animSteps = 1;

    if (currentState == NARUKAMI_INTRO) {
        maxFrame = GetMaxFrameForState(NARUKAMI_INTRO);
        if (maxFrame < 1) {
            CompleteToStance();
            UpdateScaledHurtbox();
            return;
        }
        if (introDisplayHold > 0) {
            currentFrame = introLastFrame;
            introDisplayHold -= animSteps;
            if (introDisplayHold <= 0) {
                introDisplayHold = 0;
                BeginIntroDiscard();
            }
            UpdateScaledHurtbox();
            return;
        }
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, kIntroTicks, maxFrame)) {
            introLastFrame = (maxFrame > 0) ? (maxFrame - 1) : 0;
            currentFrame = introLastFrame;
            introDisplayHold = kIntroEndHoldFrames;
        }
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == NARUKAMI_INTRO_DISCARD) {
        maxFrame = GetMaxFrameForState(NARUKAMI_INTRO_DISCARD);
        if (maxFrame < 1) maxFrame = 1;
        const float behind = -(float)facingDirection;
        discardPos.x += behind * NARUKAMI_DISCARD_FLY_X * (float)animSteps;
        discardPos.y -= NARUKAMI_DISCARD_FLY_Y * (float)animSteps;
        introDiscardSteps += animSteps;
        AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, kIntroTicks, maxFrame);
        const bool offScreen =
            discardPos.x < -80.0f || discardPos.x > (float)SCREEN_WIDTH + 80.0f ||
            discardPos.y < -80.0f ||
            introDiscardSteps >= 90;
        if (offScreen) {
            discardFlying = false;
            introDiscardSteps = 0;
            CompleteToStance();
        }
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == NARUKAMI_DAMAGE || isHit) {
        if (currentState != NARUKAMI_DAMAGE) {
            BeginHitReaction();
        }

        ApplyGravity(steps);
        PinFighterToGround(position, verticalVelocity);

        maxFrame = GetMaxFrameForState(NARUKAMI_DAMAGE);
        if (maxFrame < 1) maxFrame = 1;

        if (IsFighterAtGroundLevel(position)) {
            currentFrame = maxFrame - 1;
            damageGroundHold += animSteps;
            if (damageGroundHold >= kDamageGroundHoldTicks) {
                BeginRecover();
            }
        }
        else if (currentFrame < maxFrame - 1) {
            AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, kDamageAnimTicks, maxFrame);
            damageGroundHold = 0;
        }
        else {
            currentFrame = maxFrame - 1;
            damageGroundHold = 0;
        }
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == NARUKAMI_RECOVER) {
        maxFrame = GetMaxFrameForState(NARUKAMI_RECOVER);
        if (!g_Recover.texture || maxFrame < 1) {
            FinishRecover();
            return;
        }
        PinFighterToGround(position, verticalVelocity);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, kRecoverAnimTicks, maxFrame)) {
            FinishRecover();
        }
        UpdateScaledHurtbox();
        return;
    }

    // Stance / idle wait loop (no player input on sandbag).
    // Do not call EnterState every frame — that can reset anim and look like flicker.
    if (currentState == NARUKAMI_IDLE) {
        maxFrame = GetMaxFrameForState(NARUKAMI_IDLE);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, kIdlePlayTicks, maxFrame)) {
            currentState = NARUKAMI_STANCE;
            currentFrame = 0;
            animAccumulator = 0;
            idleWaitFrames = 0;
            maxFrame = GetMaxFrameForState(NARUKAMI_STANCE);
            if (maxFrame < 1) maxFrame = 1;
        }
        UpdateScaledHurtbox();
        return;
    }

    currentState = NARUKAMI_STANCE;
    maxFrame = GetMaxFrameForState(NARUKAMI_STANCE);
    if (maxFrame < 1) maxFrame = 1;
    AdvanceLoopFrame(animAccumulator, currentFrame, animSteps, kLoopSlowTicks, maxFrame);
    idleWaitFrames += steps;
    if (idleWaitFrames >= kIdleWaitFrames) {
        idleWaitFrames = 0;
        currentState = NARUKAMI_IDLE;
        currentFrame = 0;
        animAccumulator = 0;
        maxFrame = GetMaxFrameForState(NARUKAMI_IDLE);
        if (maxFrame < 1) maxFrame = 1;
    }
    UpdateScaledHurtbox();
}

void Narukami::UpdateHuman(int steps) {
    Fighter* opponent = GetOpponent(*this);
    if (!opponent) return;

    if (!IsHumanControlled()) {
        facingDirection = (opponent->GetPosition().x >= position.x) ? 1 : -1;
    }

    const int animSteps = 1;
    const bool attackDownNow = IsGameMouseDown(VK_LBUTTON);
    const bool attackJustPressed = attackDownNow && !attackButtonHeld;
    attackButtonHeld = attackDownNow;
    const bool isAttackPressed = IsHumanControlled() ? attackJustPressed : attackDownNow;

    if (IsSummonState(currentState)) {
        UpdateSummon(steps, *opponent);
        UpdateScaledHurtbox();
        return;
    }

    bool isRunning = IsGameKeyDown(DIK_LSHIFT) || IsGameKeyDown(DIK_RSHIFT);
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

    if (AllowsMovement(currentState) && isMoving && currentState != NARUKAMI_DASH) {
        TryApplyHorizontalDelta(moveDirX * currentVelocity * steps);
        ClampMakotoCenterX(position.x);
    }

    if (currentState == NARUKAMI_INTRO) {
        maxFrame = GetMaxFrameForState(NARUKAMI_INTRO);
        if (maxFrame < 1) maxFrame = 1;
        if (introDisplayHold > 0) {
            currentFrame = introLastFrame;
            introDisplayHold -= animSteps;
            if (introDisplayHold <= 0) {
                introDisplayHold = 0;
                BeginIntroDiscard();
            }
            UpdateScaledHurtbox();
            return;
        }
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, kIntroTicks, maxFrame)) {
            introLastFrame = (maxFrame > 0) ? (maxFrame - 1) : 0;
            currentFrame = introLastFrame;
            introDisplayHold = kIntroEndHoldFrames;
        }
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == NARUKAMI_INTRO_DISCARD) {
        maxFrame = GetMaxFrameForState(NARUKAMI_INTRO_DISCARD);
        if (maxFrame < 1) maxFrame = 1;
        const float behind = -(float)facingDirection;
        discardPos.x += behind * NARUKAMI_DISCARD_FLY_X * (float)animSteps;
        discardPos.y -= NARUKAMI_DISCARD_FLY_Y * (float)animSteps;
        introDiscardSteps += animSteps;
        AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, kIntroTicks, maxFrame);
        const bool offScreen =
            discardPos.x < -80.0f || discardPos.x > (float)SCREEN_WIDTH + 80.0f ||
            discardPos.y < -80.0f ||
            introDiscardSteps >= 90;
        if (offScreen) {
            discardFlying = false;
            introDiscardSteps = 0;
            CompleteToStance();
        }
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == NARUKAMI_PERSONA_SUMMON || currentState == NARUKAMI_PERSONA_AIR_SUMMON) {
        // Keep air-summon floating so follow-up izanagi_air_attack is not land-cancelled.
        if (currentState == NARUKAMI_PERSONA_AIR_SUMMON) {
            if (position.y > CHARACTER_GROUND_Y - 35.0f * GetCharacterRenderScale()) {
                position.y = CHARACTER_GROUND_Y - NARUKAMI_AIR_LIFT_MID * GetCharacterRenderScale();
            }
            verticalVelocity = 0.0f;
        }
        maxFrame = GetMaxFrameForState(currentState);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, MAKOTO_SUMMON_AIR_TICKS, maxFrame)) {
            const int followUp = (pendingAttackState != 0) ? pendingAttackState : NARUKAMI_ATTACK;
            pendingAttackState = 0;
            showIzanagi = true;
            izanagiFrame = 0;
            if (followUp == NARUKAMI_NEUTRAL_AIR) {
                position.y = CHARACTER_GROUND_Y - NARUKAMI_AIR_LIFT_TALL * GetCharacterRenderScale();
                verticalVelocity = 0.0f;
                jumpCount = 1;
            }
            EnterState(followUp);
        }
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == NARUKAMI_TAUNT) {
        maxFrame = GetMaxFrameForState(NARUKAMI_TAUNT);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, kIdlePlayTicks, maxFrame)) {
            CompleteToStance();
        }
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == NARUKAMI_IDLE) {
        const bool checkInput = isMoving ||
            IsGameKeyDown(DIK_SPACE) ||
            IsGameKeyDown(DIK_J) ||
            attackDownNow ||
            IsGameKeyDown(DIK_I) ||
            IsGameKeyDown(DIK_C) ||
            IsGameKeyDown(DIK_E) ||
            IsGameKeyDown(DIK_R) ||
            IsGameKeyDown(DIK_G) ||
            IsGameKeyDown(DIK_S) ||
            IsGameKeyDown(DIK_T) ||
            IsGameKeyDown(DIK_W) ||
            IsGameKeyDown(DIK_1) ||
            IsGameKeyDown(DIK_2) ||
            IsGameKeyDown(DIK_3) ||
            IsGameKeyDown(DIK_4) ||
            IsGameKeyDown(DIK_5);
        if (checkInput) {
            CompleteToStance();
            UpdateScaledHurtbox();
            return;
        }
        maxFrame = GetMaxFrameForState(NARUKAMI_IDLE);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, kIdlePlayTicks, maxFrame)) {
            CompleteToStance();
        }
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == NARUKAMI_DASH) {
        TryApplyHorizontalDelta((float)facingDirection * (velocity * kDashSpeedMultiplier) * steps);
        ClampMakotoCenterX(position.x);
        maxFrame = GetMaxFrameForState(NARUKAMI_DASH);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, kActionTicks, maxFrame)) {
            dashHasHit = false;
            CompleteToStance();
        }
        CheckAttackCollision(*opponent);
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == NARUKAMI_GUARD_AIR) {
        ApplyGravity(steps);
        maxFrame = GetMaxFrameForState(NARUKAMI_GUARD_AIR);
        AdvanceLoopFrame(animAccumulator, currentFrame, animSteps, kLoopSlowTicks, maxFrame);

        if (!IsHoldingGuardInput(*this)) {
            if (IsOnGround()) {
                jumpCount = 0;
                verticalVelocity = 0.0f;
                position.y = CHARACTER_GROUND_Y;
                CompleteToStance();
            }
            else {
                EnterState(NARUKAMI_JUMP);
            }
        }
        else if (IsOnGround() && verticalVelocity >= 0.0f) {
            jumpCount = 0;
            verticalVelocity = 0.0f;
            position.y = CHARACTER_GROUND_Y;
            EnterState(NARUKAMI_GUARD);
        }

        ClampMakotoCenterX(position.x);
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == NARUKAMI_JUMP) {
        bool isLeftPressed = IsGameKeyDown(DIK_LEFT) || IsGameKeyDown(DIK_A);
        bool isRightPressed = IsGameKeyDown(DIK_RIGHT) || IsGameKeyDown(DIK_D);
        bool isJumpPressed = IsGameKeyDown(DIK_SPACE);
        bool isAttackPressed = attackDownNow;

        if (isLeftPressed) {
            facingDirection = -1;
            jumpHorizontalSpeed = -currentVelocity * kAirControlMultiplier;
        }
        else if (isRightPressed) {
            facingDirection = 1;
            jumpHorizontalSpeed = currentVelocity * kAirControlMultiplier;
        }

        if (IsHoldingGuardInput(*this)) {
            HoldGuardState(true);
            UpdateScaledHurtbox();
            return;
        }

        if (isAttackPressed) {
            // Space / airborne + LMB → air_combo (no persona summon).
            showIzanagi = false;
            EnterState(NARUKAMI_NEUTRAL_AIR);
            UpdateScaledHurtbox();
            return;
        }

        // Double jump: Space released then pressed again while airborne.
        if (!isJumpPressed) jumpSpaceWasReleased = true;
        if (isJumpPressed && jumpSpaceWasReleased && jumpCount < 2) {
            jumpCount = 2;
            verticalVelocity = kJumpVelocity;
            currentFrame = 0;
            animAccumulator = 0;
            jumpSpaceWasReleased = false;
        }

        TryApplyHorizontalDelta(jumpHorizontalSpeed * steps);
        ClampMakotoCenterX(position.x);
        ApplyGravity(steps);

        maxFrame = GetMaxFrameForState(NARUKAMI_JUMP);
        AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, kActionTicks, maxFrame);

        if (IsOnGround() && verticalVelocity >= 0.0f) {
            jumpCount = 0;
            CompleteToStance();
        }
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == NARUKAMI_NEUTRAL_AIR) {
        // Persona air skill (key 4): hold height until izanagi_air_attack finishes.
        if (showIzanagi) {
            verticalVelocity = 0.0f;
            if (position.y > CHARACTER_GROUND_Y - 40.0f * GetCharacterRenderScale()) {
                position.y = CHARACTER_GROUND_Y - NARUKAMI_AIR_LIFT_TALL * GetCharacterRenderScale();
            }
        }
        else {
            ApplyGravity(steps);
        }
        maxFrame = GetMaxFrameForState(NARUKAMI_NEUTRAL_AIR);
        const bool animDone = AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, kActionTicks, maxFrame);
        const bool landedEarly = !showIzanagi && IsOnGround() && verticalVelocity >= 0.0f;
        if (animDone || landedEarly) {
            position.y = CHARACTER_GROUND_Y;
            verticalVelocity = 0.0f;
            jumpCount = 0;
            CompleteToStance();
        }
        else {
            CheckAttackCollision(*opponent);
        }
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == NARUKAMI_ATTACK ||
        currentState == NARUKAMI_CROUCH_ATTACK ||
        currentState == NARUKAMI_SIDE_ATTACK ||
        currentState == NARUKAMI_ATTACK_UP ||
        currentState == NARUKAMI_DOWN_ATTACK ||
        IsSpecialMeleeState(currentState)) {
        maxFrame = GetMaxFrameForState(currentState);
        const int ticks = (currentState == NARUKAMI_ATTACK_UP) ? kCrossSlashTicks : kActionTicks;
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, ticks, maxFrame)) {
            if (currentState == NARUKAMI_CROUCH_ATTACK && IsGameKeyDown(DIK_C)) {
                EnterState(NARUKAMI_CROUCH);
                if (maxFrame > 0) currentFrame = maxFrame - 1;
            }
            else {
                CompleteToStance();
            }
        }
        CheckAttackCollision(*opponent);
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == NARUKAMI_DAMAGE) {
        ApplyGravity(steps);
        PinFighterToGround(position, verticalVelocity);
        maxFrame = GetMaxFrameForState(NARUKAMI_DAMAGE);
        if (maxFrame < 1) maxFrame = 1;

        if (IsFighterAtGroundLevel(position)) {
            currentFrame = maxFrame - 1;
            damageGroundHold += animSteps;
            if (damageGroundHold >= kDamageGroundHoldTicks || hitStunTimer <= 0) {
                BeginRecover();
            }
            hitStunTimer -= animSteps;
        }
        else if (currentFrame < maxFrame - 1) {
            AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, kDamageAnimTicks, maxFrame);
            damageGroundHold = 0;
        }
        else {
            currentFrame = maxFrame - 1;
            damageGroundHold = 0;
        }
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == NARUKAMI_RECOVER) {
        maxFrame = GetMaxFrameForState(NARUKAMI_RECOVER);
        PinFighterToGround(position, verticalVelocity);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, kRecoverAnimTicks, maxFrame)) {
            FinishRecover();
        }
        UpdateScaledHurtbox();
        return;
    }

    const bool isJumpPressed = IsGameKeyDown(DIK_SPACE);
    const bool isDashHeld = IsGameKeyDown(DIK_J);
    const bool isGuardPressed = IsHoldingGuardInput(*this);
    const bool isCrouchPressed = IsGameKeyDown(DIK_C);
    const bool isBigGamblePressed = IsGameKeyDown(DIK_E);
    const bool isCrossSlashPressed = IsGameKeyDown(DIK_R);
    const bool isLightningPressed = IsGameKeyDown(DIK_G);
    const bool isSwiftHeld = IsGameKeyDown(DIK_S);
    const bool isRagingHeld = IsGameKeyDown(DIK_W);
    const bool isTauntPressed = IsGameKeyDown(DIK_T);
    const bool isKey1 = IsGameKeyDown(DIK_1);
    const bool isKey2 = IsGameKeyDown(DIK_2);
    const bool isKey3 = IsGameKeyDown(DIK_3);
    const bool isKey4 = IsGameKeyDown(DIK_4);
    const bool isKey5 = IsGameKeyDown(DIK_5);

    const bool hasAnyInput = isMoving || isJumpPressed || isDashHeld || attackDownNow ||
        isGuardPressed || isCrouchPressed || isBigGamblePressed || isCrossSlashPressed ||
        isLightningPressed || isSwiftHeld || isRagingHeld || isTauntPressed ||
        isKey1 || isKey2 || isKey3 || isKey4 || isKey5;

    if (hasAnyInput) noInputFrames = 0;
    else if (currentState == NARUKAMI_STANCE) noInputFrames += steps;

    int nextState = NARUKAMI_STANCE;

    // Keys 1–5 consume SP.
    if (isKey1 && !IsSummonState(currentState) && TryConsumeSp(kSpCostZio)) {
        BeginSummon(NARUKAMI_SUMMON_ZIO);
        UpdateScaledHurtbox();
        return;
    }
    if (isKey2 && !IsSummonState(currentState) && TryConsumeSp(kSpCostZiodyne)) {
        BeginSummon(NARUKAMI_SUMMON_ZIODYNE);
        UpdateScaledHurtbox();
        return;
    }
    if (isKey5 && !IsSummonState(currentState) && TryConsumeSp(kSpCostMyriad)) {
        BeginSummon(NARUKAMI_MYRIAD_TRUTHS);
        UpdateScaledHurtbox();
        return;
    }
    if (isKey3 && TryConsumeSp(kSpCostPersonaGround)) {
        BeginPersonaSummon(NARUKAMI_PERSONA_SUMMON, NARUKAMI_ATTACK);
        UpdateScaledHurtbox();
        return;
    }
    if (isKey4 && TryConsumeSp(kSpCostPersonaAir)) {
        if (IsOnGround()) {
            position.y = CHARACTER_GROUND_Y - NARUKAMI_AIR_LIFT_SHORT * GetCharacterRenderScale();
            verticalVelocity = 0.0f;
            jumpCount = 1;
        }
        BeginPersonaSummon(NARUKAMI_PERSONA_AIR_SUMMON, NARUKAMI_NEUTRAL_AIR);
        UpdateScaledHurtbox();
        return;
    }

    if (isTauntPressed) {
        nextState = NARUKAMI_TAUNT;
    }
    else if (isJumpPressed && isAttackPressed) {
        nextState = NARUKAMI_NEUTRAL_AIR;
        showIzanagi = false;
        if (IsOnGround()) {
            position.y = CHARACTER_GROUND_Y - NARUKAMI_AIR_LIFT_SHORT * GetCharacterRenderScale();
            verticalVelocity = 0.0f;
            jumpCount = 1;
        }
    }
    else if (isLightningPressed && TryConsumeStamina(STAMINA_COST_ACTION)) {
        // G → lightning_flash
        nextState = NARUKAMI_DOWN_ATTACK;
        showIzanagi = false;
    }
    else if (isRagingHeld && isAttackPressed && TryConsumeStamina(STAMINA_COST_ACTION)) {
        // W + LMB → raging_lion
        nextState = NARUKAMI_RAGING_LION;
        showIzanagi = false;
    }
    else if (isSwiftHeld && isAttackPressed && TryConsumeStamina(STAMINA_COST_ACTION)) {
        // S + LMB → swift_strike + izanagi
        nextState = NARUKAMI_SIDE_ATTACK;
    }
    else if (isBigGamblePressed && TryConsumeStamina(STAMINA_COST_ACTION)) {
        // E → big_gamble
        nextState = NARUKAMI_BIG_GAMBLE;
        showIzanagi = false;
    }
    else if (!IsHumanControlled() && isBigGamblePressed) {
        nextState = NARUKAMI_ATTACK;
        showIzanagi = false;
    }
    else if (isCrossSlashPressed && TryConsumeStamina(STAMINA_COST_ACTION)) {
        // R → cross_slash + izanagi
        nextState = NARUKAMI_ATTACK_UP;
    }
    else if (!IsHumanControlled() && isCrossSlashPressed) {
        nextState = NARUKAMI_ATTACK;
        showIzanagi = false;
    }
    else if (isDashHeld && TryConsumeStamina(STAMINA_COST_ACTION)) {
        // J → dash
        nextState = NARUKAMI_DASH;
        dashHasHit = false;
    }
    else if (isJumpPressed && IsOnGround()) {
        nextState = NARUKAMI_JUMP;
        jumpCount = 1;
        jumpSpaceWasReleased = false;
        verticalVelocity = kJumpVelocity;
        jumpHorizontalSpeed = (moveDirX != 0.0f)
            ? (moveDirX * currentVelocity * kAirControlMultiplier)
            : 0.0f;
    }
    else if (isCrouchPressed && isAttackPressed) {
        nextState = NARUKAMI_CROUCH_ATTACK;
        showIzanagi = false;
    }
    else if (isAttackPressed) {
        nextState = NARUKAMI_ATTACK;
        showIzanagi = false;
    }
    else if (isMoving) {
        nextState = isRunning ? NARUKAMI_RUN : NARUKAMI_WALK;
    }
    else if (isGuardPressed) {
        nextState = IsOnGround() ? NARUKAMI_GUARD : NARUKAMI_GUARD_AIR;
    }
    else if (isCrouchPressed && !isMoving) {
        nextState = NARUKAMI_CROUCH;
    }
    else if (noInputFrames >= IDLE_THRESHOLD_FRAMES) {
        nextState = NARUKAMI_IDLE;
        noInputFrames = 0;
    }
    else {
        nextState = NARUKAMI_STANCE;
    }

    if (currentState != nextState) {
        EnterState(nextState);
        if (nextState == NARUKAMI_IDLE && IsOnGround()) {
            position.y = CHARACTER_GROUND_Y;
        }
        if (nextState == NARUKAMI_SIDE_ATTACK || nextState == NARUKAMI_ATTACK_UP) {
            showIzanagi = true;
        }
    }

    maxFrame = GetMaxFrameForState(currentState);
    switch (currentState) {
    case NARUKAMI_STANCE:
        AdvanceLoopFrame(animAccumulator, currentFrame, animSteps, kLoopSlowTicks, maxFrame);
        break;
    case NARUKAMI_WALK:
    case NARUKAMI_RUN:
        AdvanceLoopFrame(animAccumulator, currentFrame, animSteps, kLoopFastTicks, maxFrame);
        break;
    case NARUKAMI_GUARD:
    case NARUKAMI_GUARD_AIR:
        AdvanceLoopFrame(animAccumulator, currentFrame, animSteps, kLoopSlowTicks, maxFrame);
        break;
    case NARUKAMI_CROUCH:
        if (maxFrame > 0) {
            currentFrame = maxFrame - 1;
        }
        break;
    default:
        break;
    }

    UpdateScaledHurtbox();
}

void Narukami::Update() {
    if (IsPlayingResultPose()) {
        maxFrame = GetMaxFrameForState(currentState);
        if (maxFrame < 1) maxFrame = 1;

        int steps = g_GameTimer.GetLastFramesToUpdate();
        if (steps <= 0) steps = 1;
        if (steps > GAME_TIMER_MAX_STEPS_PER_FRAME) steps = GAME_TIMER_MAX_STEPS_PER_FRAME;

        if (currentState == NARUKAMI_LOSE) {
            const int knockdownFrames = g_Damage.maxFrame;
            if (knockdownFrames > 0) {
                maxFrame = knockdownFrames;
            }
            currentFrame = maxFrame - 1;
        }
        else if (currentState == NARUKAMI_WIN) {
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
                    if (g_Win.texture) {
                        resultPoseHoldFrame = FindLastVisibleSheetFrame(
                            g_Win.texture,
                            NARUKAMI_CELL_SIZE,
                            NARUKAMI_CELL_SIZE,
                            g_Win.cols,
                            maxFrame);
                    }
                    else {
                        resultPoseHoldFrame = maxFrame - 1;
                    }
                }
                currentFrame = resultPoseHoldFrame;
            }
        }

        position.y = CHARACTER_GROUND_Y;
        verticalVelocity = 0.0f;
        UpdateScaledHurtbox();
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

    if (Fighter* opponent = GetOpponent(*this)) {
        ClampFighterAgainstOpponent(*this, *opponent);
        UpdateScaledHurtbox();
    }
}

void Narukami::RenderSkillBackdropBeforeOpponent(LPD3DXSPRITE sprite) {
    if (!sprite || (isDead && !IsPlayingResultPose() && !IsBattleEndSequence())) return;

    const D3DCOLOR color = ApplySpriteTint(D3DCOLOR_XRGB(255, 255, 255), GetSpriteTint());

    if (showEffect && currentState == NARUKAMI_MYRIAD_TRUTHS) {
        DrawMyriadRippleWaves(sprite, effectPos, effectFrame, color);
        return;
    }

    if (!showEffect || currentState != NARUKAMI_SUMMON_ZIODYNE) return;

    Fighter* opponent = GetOpponent(*this);
    const int skillFacing = opponent
        ? GetSkillFacingDirection(position, GetEnemyHurtboxCenter(*opponent))
        : facingDirection;
    DrawNarukamiZiodyneEffect(sprite, effectPos, skillFacing, effectFrame, color);
}

void Narukami::Render(LPD3DXSPRITE sprite) {
    if (!sprite || (isDead && !IsPlayingResultPose() && !IsBattleEndSequence())) return;

    const D3DCOLOR color = ApplySpriteTint(D3DCOLOR_XRGB(255, 255, 255), GetSpriteTint());
    const bool groundedKnockdown =
        IsFighterAtGroundLevel(position) &&
        (currentState == NARUKAMI_DAMAGE || currentState == NARUKAMI_RECOVER || currentState == NARUKAMI_LOSE);
    float bodyFeetY = GetNarukamiBodyFeetY(currentState, currentFrame, groundedKnockdown);
    const NarukamiTexture* knockdownTex = GetTextureForState(currentState);
    if (knockdownTex && knockdownTex->texture &&
        (currentState == NARUKAMI_DAMAGE || currentState == NARUKAMI_RECOVER || currentState == NARUKAMI_LOSE)) {
        bodyFeetY = MeasureTextureFrameBottomY(
            knockdownTex->texture,
            currentFrame,
            NARUKAMI_CELL_SIZE,
            NARUKAMI_CELL_SIZE,
            knockdownTex->cols);
    }
    const bool frontSummon =
        showIzanagi &&
        (currentState == NARUKAMI_SUMMON_ZIO || currentState == NARUKAMI_SUMMON_ZIODYNE);

    const NarukamiTexture* izanagiTex = nullptr;
    if (currentState == NARUKAMI_SUMMON_ZIO && showIzanagi) izanagiTex = &g_IzanagiZio;
    else if (currentState == NARUKAMI_SUMMON_ZIODYNE && showIzanagi) izanagiTex = &g_IzanagiZiodyne;
    else if (currentState == NARUKAMI_MYRIAD_TRUTHS && showIzanagi) izanagiTex = &g_IzanagiMyriadTruths;
    else if (currentState == NARUKAMI_ATTACK && showIzanagi) {
        izanagiTex = &g_IzanagiAttack;
    }
    else if (currentState == NARUKAMI_NEUTRAL_AIR && showIzanagi) {
        izanagiTex = &g_IzanagiAirAttack;
    }
    else if (currentState == NARUKAMI_SIDE_ATTACK) {
        izanagiTex = &g_IzanagiSwiftStrike;
    }
    else if (currentState == NARUKAMI_ATTACK_UP) {
        izanagiTex = &g_IzanagiCrossSlash;
    }

    // Persona behind the body for melee; Myriad draws Izanagi above the head after the body.
    if (!frontSummon && izanagiTex && izanagiTex->texture && izanagiTex->maxFrame > 0 &&
        currentState != NARUKAMI_MYRIAD_TRUTHS) {
        const D3DXVECTOR3 pos = IsSummonState(currentState)
            ? izanagiPos
            : D3DXVECTOR3(
                position.x - (float)facingDirection * kIzanagiBehindX,
                position.y - kIzanagiBehindY,
                0.0f);
        int frame = IsSummonState(currentState) ? izanagiFrame : currentFrame;
        if (frame >= izanagiTex->maxFrame) {
            frame = izanagiTex->maxFrame - 1;
        }
        DrawLayer(sprite, *izanagiTex, frame, pos, facingDirection, kPersonaDrawScale, color, NARUKAMI_STANCE_FEET_Y);
    }

    // Sword hilt flies behind during discard (draw behind body).
    if (currentState == NARUKAMI_INTRO_DISCARD && discardFlying && g_IntroEffect.texture) {
        DrawLayer(sprite, g_IntroEffect, currentFrame, discardPos, facingDirection, 1.0f, color, NARUKAMI_STANCE_FEET_Y);
    }

    if (currentState == NARUKAMI_WIN && g_IzanagiWin.texture && g_IzanagiWin.maxFrame > 0) {
        int winFrame = currentFrame;
        if (winFrame >= g_IzanagiWin.maxFrame) {
            winFrame = g_IzanagiWin.maxFrame - 1;
        }
        const D3DXVECTOR3 izPos(
            position.x - (float)facingDirection * kIzanagiBehindX,
            position.y - kIzanagiBehindY,
            0.0f);
        DrawLayer(
            sprite,
            g_IzanagiWin,
            winFrame,
            izPos,
            facingDirection,
            kPersonaDrawScale,
            color,
            NARUKAMI_STANCE_FEET_Y);
    }

    // Body — during discard keep stance pose on the fighter while hilt flies away.
    if (currentState == NARUKAMI_INTRO_DISCARD) {
        if (g_Stance.texture) {
            DrawLayer(sprite, g_Stance, 0, position, facingDirection, 1.0f, color, NARUKAMI_STANCE_FEET_Y);
        }
    }
    else {
        const NarukamiTexture* bodyTex = GetTextureForState(currentState);
        if (bodyTex && bodyTex->texture) {
            DrawLayer(sprite, *bodyTex, currentFrame, position, facingDirection, 1.0f, color, bodyFeetY);
        }
    }

    if (currentState == NARUKAMI_MYRIAD_TRUTHS && showIzanagi &&
        g_IzanagiMyriadTruths.texture && g_IzanagiMyriadTruths.maxFrame > 0) {
        int frame = izanagiFrame;
        if (frame >= g_IzanagiMyriadTruths.maxFrame) {
            frame = g_IzanagiMyriadTruths.maxFrame - 1;
        }
        DrawLayer(
            sprite,
            g_IzanagiMyriadTruths,
            frame,
            izanagiPos,
            facingDirection,
            kPersonaDrawScale,
            color,
            NARUKAMI_STANCE_FEET_Y);
    }

    Fighter* opponent = GetOpponent(*this);
    const int skillFacing = opponent
        ? GetSkillFacingDirection(position, GetEnemyHurtboxCenter(*opponent))
        : facingDirection;
    const int izanagiFacing = (currentState == NARUKAMI_SUMMON_ZIO || currentState == NARUKAMI_SUMMON_ZIODYNE)
        ? skillFacing
        : facingDirection;

    // Izanagi first, then Ziodyne toward the foe (Thanatos → Maziodyne).
    if (frontSummon && izanagiTex && izanagiTex->texture && izanagiTex->maxFrame > 0) {
        int frame = izanagiFrame;
        if (frame >= izanagiTex->maxFrame) {
            frame = izanagiTex->maxFrame - 1;
        }
        DrawLayer(
            sprite,
            *izanagiTex,
            frame,
            izanagiPos,
            izanagiFacing,
            kPersonaDrawScale,
            color,
            NARUKAMI_STANCE_FEET_Y);
    }

    // Zio bolt on the foe's body center.
    if (showEffect && currentState == NARUKAMI_SUMMON_ZIO) {
        const NarukamiTexture& effectTex = g_ZioEffect;
        if (effectTex.texture) {
            SetFrameRect(g_SrcRect, effectTex, effectFrame);
            DrawScaledCharacterSprite(
                sprite,
                effectTex.texture,
                &g_SrcRect,
                effectPos,
                skillFacing,
                AGI_EFFECT_SCALE * PERSONA_EFFECT_SCALE,
                color,
                (float)MAKOTO_CELL_SIZE,
                MAKOTO_FEET_Y);
        }
    }
}

void Narukami::RenderDebugHitbox(LPD3DXSPRITE sprite) {
    if (!sprite) return;
    UpdateScaledHurtbox();
    const AABB bodyBox = GetBodyCollisionBox();
    DrawDebugRect(sprite, bodyBox.x, bodyBox.y, bodyBox.width, bodyBox.height, D3DCOLOR_ARGB(160, 255, 64, 255));
    DrawDebugRect(sprite, hurtbox.x, hurtbox.y, hurtbox.width, hurtbox.height, D3DCOLOR_ARGB(160, 80, 200, 120));

    if (!IsMeleeState(currentState)) return;

    Fighter* opponent = GetOpponent(*this);
    if (!opponent) return;

    // Recompute active melee box for debug draw.
    AttackData* data = nullptr;
    switch (currentState) {
    case NARUKAMI_ATTACK:
    case NARUKAMI_CROUCH_ATTACK:
    case NARUKAMI_NEUTRAL_AIR: data = &attackHitbox; break;
    case NARUKAMI_SIDE_ATTACK: data = &sideAttackHitbox; break;
    case NARUKAMI_ATTACK_UP: data = &attackUpHitbox; break;
    case NARUKAMI_DOWN_ATTACK: data = &downAttackHitbox; break;
    default: break;
    }
    if (!data) return;

    float attackX = 0.0f;
    float attackY = 0.0f;
    float boxW = 0.0f;
    float boxH = 0.0f;
    BuildAttackBox(position, facingDirection, GetCharacterRenderScale(), *data, attackX, attackY, boxW, boxH);
    DrawDebugRect(sprite, attackX, attackY, boxW, boxH, D3DCOLOR_ARGB(140, 255, 64, 64));
}

void Narukami::TakeDamage(int damage) {
    if (isDead) return;

    int appliedDamage = damage;
    if (TryProcessGuardBlock(*this, damage, appliedDamage)) {
        if (appliedDamage > 0) {
            health -= appliedDamage;
            if (health < 0) health = 0;
        }
        UpdateScaledHurtbox();
        return;
    }

    if (!IsHumanControlled()) {
        // Always apply HP; only skip re-starting damage anim while already downed.
        health -= appliedDamage;
        if (health < 0) health = 0;
        if (currentState != NARUKAMI_DAMAGE && currentState != NARUKAMI_RECOVER) {
            BeginHitReaction();
        }
        if (!TRAINING_MODE && health <= 0) {
            isDead = true;
        }
        return;
    }

    health -= appliedDamage;
    if (health < 0) health = 0;
    if (currentState != NARUKAMI_DAMAGE && currentState != NARUKAMI_RECOVER && !IsSummonState(currentState)) {
        BeginHitReaction();
    }
    else {
        isHit = true;
        hitStunTimer = kHitStunFrames;
    }

    if (!TRAINING_MODE && health <= 0) {
        isDead = true;
    }
}

bool Narukami::IsInGuardState() const {
    return currentState == NARUKAMI_GUARD || currentState == NARUKAMI_GUARD_AIR;
}

void Narukami::HoldGuardState(bool airborne) {
    const int target = airborne ? NARUKAMI_GUARD_AIR : NARUKAMI_GUARD;
    if (currentState == target) {
        return;
    }
    EnterState(target);
}

void Narukami::ApplySkillDamage(int damage) {
    // Makoto/Joker style: skills always deal HP even during opponent damage/recover.
    if (isDead) return;
    health -= damage;
    if (health < 0) health = 0;

    if (currentState == NARUKAMI_DAMAGE || currentState == NARUKAMI_RECOVER) {
        isHit = true;
        hitStunTimer = kHitStunFrames;
    }
    else if (!IsSummonState(currentState)) {
        BeginHitReaction();
    }

    if (!TRAINING_MODE && health <= 0) {
        isDead = true;
    }
}

void Narukami::BeginVictoryPose() {
    isHit = false;
    hitStunTimer = 0;
    resultPoseAnimLocked = false;
    resultPoseHoldFrame = -1;
    showIzanagi = false;
    showEffect = false;
    currentState = NARUKAMI_WIN;
    currentFrame = 0;
    animAccumulator = 0;
    maxFrame = GetMaxFrameForState(NARUKAMI_WIN);
    if (maxFrame < 1) maxFrame = 1;
    position.y = CHARACTER_GROUND_Y;
    verticalVelocity = 0.0f;
    UpdateScaledHurtbox();
}

void Narukami::BeginDefeatPose() {
    isHit = false;
    hitStunTimer = 0;
    resultPoseHoldFrame = -1;
    showIzanagi = false;
    showEffect = false;
    currentState = NARUKAMI_LOSE;
    animAccumulator = 0;
    damageGroundHold = 0;
    currentFrame = 0;

    const int knockdownFrames = g_Damage.maxFrame;
    maxFrame = (knockdownFrames > 0) ? knockdownFrames : 1;
    currentFrame = maxFrame - 1;

    position.y = CHARACTER_GROUND_Y;
    verticalVelocity = 0.0f;
    jumpCount = 0;
    UpdateScaledHurtbox();
}

bool Narukami::IsPlayingResultPose() const {
    return currentState == NARUKAMI_WIN || currentState == NARUKAMI_LOSE;
}

bool Narukami::IsInCombatAction() const {
    switch (currentState) {
    case NARUKAMI_ATTACK: case NARUKAMI_CROUCH_ATTACK: case NARUKAMI_SIDE_ATTACK:
    case NARUKAMI_ATTACK_UP: case NARUKAMI_DOWN_ATTACK: case NARUKAMI_NEUTRAL_AIR:
    case NARUKAMI_DASH: case NARUKAMI_JUMP:
    case NARUKAMI_PERSONA_SUMMON: case NARUKAMI_PERSONA_AIR_SUMMON:
    case NARUKAMI_SUMMON_ZIO: case NARUKAMI_SUMMON_ZIODYNE:
    case NARUKAMI_RAGING_LION: case NARUKAMI_BIG_GAMBLE: case NARUKAMI_MYRIAD_TRUTHS:
        return true;
    default:
        return false;
    }
}

void Narukami::Reset() {
    ApplySlotSpawnDefaults();
    spawnPosition = position;
    health = maxHealth;
    sp = 0;
    RefillStamina();
    isDead = false;
    resultPoseAnimLocked = false;
    isHit = false;
    hitStunTimer = 0;
    currentState = NARUKAMI_INTRO;
    currentFrame = 0;
    animAccumulator = 0;
    maxFrame = GetMaxFrameForState(NARUKAMI_INTRO);
    jumpCount = 0;
    jumpHorizontalSpeed = 0.0f;
    verticalVelocity = 0.0f;
    hitThisAttack = false;
    dashHasHit = false;
    attackButtonHeld = false;
    skillHit = false;
    jumpSpaceWasReleased = true;
    personaAnimAccumulator = 0;
    izanagiFrame = 0;
    effectFrame = 0;
    showIzanagi = false;
    showEffect = false;
    noInputFrames = 0;
    idleWaitFrames = 0;
    introDisplayHold = 0;
    introLastFrame = 0;
    introDiscardSteps = 0;
    pendingAttackState = 0;
    discardFlying = false;
    damageGroundHold = 0;
    UpdateScaledHurtbox();
}

bool LoadNarukamiTextures() {
    struct TextureLoadInfo {
        NarukamiTexture* tex;
        const char* path;
        int frameCount; // 0 = use full grid from texture size
    };

    // frameCount = usable cells with art only (empty trailing cells cause flicker).
    TextureLoadInfo textures[] = {
        { &g_Stance, "assets/narukami/stance.png", 4 },
        { &g_Walk, "assets/narukami/walk.png", 8 },
        { &g_Run, "assets/narukami/run.png", 8 },
        { &g_Dash, "assets/narukami/dash.png", 2 },
        { &g_Jump, "assets/narukami/jump.png", 4 },
        { &g_Crouch, "assets/narukami/crouch.png", 2 },
        { &g_Guard, "assets/narukami/guard.png", 2 },
        { &g_Attack, "assets/narukami/attack_combo.png", 19 },
        { &g_SideAttack, "assets/narukami/swift_strike.png", 5 },
        { &g_UpAttack, "assets/narukami/cross_slash.png", 5 },
        { &g_DownAttack, "assets/narukami/lightning_flash.png", 3 },
        { &g_AirAttack, "assets/narukami/air_combo.png", 14 },
        { &g_Damage, "assets/narukami/damage.png", 7 },
        { &g_Recover, "assets/narukami/recover.png", 3 },
        { &g_Win, "assets/narukami/win.png", 12 },
        { &g_IzanagiWin, "assets/narukami/izanagi_win.png", 12 },
        { &g_Intro, "assets/narukami/intro.png", 13 },
        { &g_IntroEffect, "assets/narukami/intro_effect.png", 2 },
        { &g_Taunt, "assets/narukami/taunt.png", 7 },
        { &g_NarukamiZio, "assets/narukami/narukami_zio.png", 3 },
        { &g_NarukamiZiodyne, "assets/narukami/narukami_ziodyne.png", 6 },
        { &g_IzanagiZio, "assets/narukami/izanagi_zio.png", 3 },
        { &g_IzanagiZiodyne, "assets/narukami/izanagi_ziodyne.png", 3 },
        { &g_ZioEffect, "assets/narukami/zio.png", 3 },
        { &g_ZiodyneEffect, "assets/narukami/ziodyne.png", 4 },
        { &g_IzanagiAttack, "assets/narukami/izanagi_attack.png", 9 },
        { &g_IzanagiAirAttack, "assets/narukami/izanagi_air_attack.png", 9 },
        { &g_IzanagiSwiftStrike, "assets/narukami/izanagi_swift_strike.png", 3 },
        { &g_IzanagiCrossSlash, "assets/narukami/izanagi_cross_slash.png", 4 },
        { &g_PersonaAttack, "assets/narukami/persona_attack.png", 6 },
        { &g_PersonaAirAttack, "assets/narukami/persona_air_attack.png", 6 },
        { &g_RagingLion, "assets/narukami/raging_lion.png", 6 },
        { &g_BigGamble, "assets/narukami/big_gamble.png", 6 },
        { &g_MyriadTruths, "assets/narukami/myriad_truths.png", 9 },
        { &g_IzanagiMyriadTruths, "assets/narukami/izanagi_myriad_truths.png", 17 },
    };

    for (int i = 0; i < (int)(sizeof(textures) / sizeof(textures[0])); ++i) {
        if (!LoadSheet(*textures[i].tex, textures[i].path, textures[i].frameCount)) {
            char msg[512];
            sprintf_s(msg, "Failed to load %s", textures[i].path);
            MessageBox(g_hWnd, msg, "Narukami Texture Error", MB_OK);
            return false;
        }
    }

    if (!LoadSheet(g_GuardAir, "assets/narukami/guard_air.png", 3)) {
        g_GuardAir = g_Guard;
    }

    return true;
}

void CleanUpNarukamiTextures() {
    ReleaseSheet(g_Stance);
    ReleaseSheet(g_Walk);
    ReleaseSheet(g_Run);
    ReleaseSheet(g_Dash);
    ReleaseSheet(g_Jump);
    ReleaseSheet(g_Crouch);
    ReleaseSheet(g_Guard);
    if (g_GuardAir.texture && g_GuardAir.texture != g_Guard.texture) {
        ReleaseSheet(g_GuardAir);
    }
    ReleaseSheet(g_Attack);
    ReleaseSheet(g_SideAttack);
    ReleaseSheet(g_UpAttack);
    ReleaseSheet(g_DownAttack);
    ReleaseSheet(g_AirAttack);
    ReleaseSheet(g_Damage);
    ReleaseSheet(g_Recover);
    ReleaseSheet(g_Win);
    ReleaseSheet(g_IzanagiWin);
    ReleaseSheet(g_Intro);
    ReleaseSheet(g_IntroEffect);
    ReleaseSheet(g_Taunt);
    ReleaseSheet(g_NarukamiZio);
    ReleaseSheet(g_NarukamiZiodyne);
    ReleaseSheet(g_IzanagiZio);
    ReleaseSheet(g_IzanagiZiodyne);
    ReleaseSheet(g_ZioEffect);
    ReleaseSheet(g_ZiodyneEffect);
    ReleaseSheet(g_IzanagiAttack);
    ReleaseSheet(g_IzanagiAirAttack);
    ReleaseSheet(g_IzanagiSwiftStrike);
    ReleaseSheet(g_IzanagiCrossSlash);
    ReleaseSheet(g_PersonaAttack);
    ReleaseSheet(g_PersonaAirAttack);
    ReleaseSheet(g_RagingLion);
    ReleaseSheet(g_BigGamble);
    ReleaseSheet(g_MyriadTruths);
    ReleaseSheet(g_IzanagiMyriadTruths);
}
