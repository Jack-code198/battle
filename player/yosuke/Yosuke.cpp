#include "Yosuke.h"
#include "../../ai.h"
#include "../../config.h"
#include "../../renderer.h"
#include "../../game_logic.h"
#include "../../input.h"
#include "../../audio.h"
#include "../../collision.h"
#include <cmath>
#include <optional>
#include <stdio.h>

extern AttackData attackHitbox;
extern AttackData sideAttackHitbox;
extern AttackData attackUpHitbox;
extern AttackData downAttackHitbox;

namespace {

constexpr int kCellSize = MAKOTO_CELL_SIZE;
constexpr int kIntroTicks = MAKOTO_INTRO_TICKS;
constexpr int kActionTicks = MAKOTO_ACTION_TICKS;
constexpr int kWalkAnimTicks = MAKOTO_LOOP_TICKS_FAST;
constexpr int kRunAnimTicks = MAKOTO_LOOP_TICKS_FAST;
constexpr int kLocomotionFrameCount = 4;
constexpr int kCrescentSlashTicks = NARUKAMI_CROSS_SLASH_TICKS;
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
constexpr int kIntroEndHoldFrames = INTRO_END_HOLD_FRAMES;
constexpr int kGarudyneSummonTicks = NARUKAMI_MYRIAD_SUMMON_TICKS;
constexpr int kGarudynePersonaTicks = NARUKAMI_MYRIAD_PERSONA_TICKS;
constexpr int kSkillHitStartFrame = 1;

constexpr float kJumpVelocity = FIGHTER_JUMP_VELOCITY;
constexpr float kAirControlMultiplier = NARUKAMI_AIR_CONTROL_MULTIPLIER;
constexpr float kDashSpeedMultiplier = MAKOTO_DASH_SPEED_MULTIPLIER;

constexpr int kPersonaStrikeDamage = 50;
constexpr int kMirageSlashDamage = 40;
constexpr int kBraveBladeDamage = 55;
constexpr int kGarudyneDamage = 70;
constexpr int kFlyingKunaiDamage = 45;
constexpr int kCrescentSlashDamage = 35;
constexpr int kMoonsaultDamage = 40;

constexpr float kGarudyneSpinSpeed = 14.0f;
constexpr float kGarudyneArriveRadius = 20.0f;
constexpr float kYosukeMeleeReachBonus = 36.0f;
constexpr float kYosukeSkillProximityX = 96.0f;

constexpr int kSpCostPersona = SP_COST_SUMMON_1;
constexpr int kSpCostMirage = SP_COST_SUMMON_2;
constexpr int kSpCostBraveBlade = SP_COST_THANATOS_SLASH;
constexpr int kSpCostGarudyne = SP_COST_SUMMON_AIR;

constexpr float kJiraiyaBehindX = PERSONA_BEHIND_HORIZONTAL * 0.65f;
constexpr float kJiraiyaBehindY = ORPHEUS_BEHIND_VERTICAL * 0.55f;
constexpr float kMirageFrontOffset = 48.0f;
constexpr float kJiraiyaDrawScale = YOSUKE_JIRAIYA_SCALE;

struct YosukeTexture {
    LPDIRECT3DTEXTURE9 texture = nullptr;
    int cols = 1;
    int rows = 1;
    int maxFrame = 1;
};

YosukeTexture g_Stance;
YosukeTexture g_Walk;
YosukeTexture g_Run;
YosukeTexture g_Dash;
YosukeTexture g_BackDash;
YosukeTexture g_Jump;
YosukeTexture g_Guard;
YosukeTexture g_GuardAir;
YosukeTexture g_Attack;
YosukeTexture g_AirCombo;
YosukeTexture g_CrescentSlash;
YosukeTexture g_Moonsault;
YosukeTexture g_FlyingKunai;
YosukeTexture g_MirageSlash;
YosukeTexture g_BraveBlade;
YosukeTexture g_Garudyne;
YosukeTexture g_PersonaAttack;
YosukeTexture g_Damage;
YosukeTexture g_Recover;
YosukeTexture g_Intro;
YosukeTexture g_Win;
YosukeTexture g_JiraiyaAttack;
YosukeTexture g_JiraiyaMirageSlash;
YosukeTexture g_JiraiyaBraveBlade;
YosukeTexture g_JiraiyaGarudyne;
YosukeTexture g_JiraiyaWin;
YosukeTexture g_FlyingKunaiEffect;
YosukeTexture g_JiraiyaAttackEffect;

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

int ClampFrameIndex(const YosukeTexture& tex, int frameIndex) {
    if (tex.maxFrame <= 0) return 0;
    if (frameIndex < 0) return 0;
    if (frameIndex >= tex.maxFrame) return tex.maxFrame - 1;
    return frameIndex;
}

void SetFrameRect(RECT& rect, const YosukeTexture& tex, int frameIndex) {
    const int frame = ClampFrameIndex(tex, frameIndex);
    rect.left = kCellSize * (frame % tex.cols);
    rect.top = kCellSize * (frame / tex.cols);
    rect.right = rect.left + kCellSize;
    rect.bottom = rect.top + kCellSize;
}

D3DXVECTOR3 GetEnemyHurtboxCenter(const Fighter& enemy) {
    const AABB& hb = enemy.GetHurtbox();
    return D3DXVECTOR3(hb.x + hb.width * 0.5f, hb.y + hb.height * 0.5f, 0.0f);
}

D3DXVECTOR3 GetSkillEffectPos(const Fighter& enemy) {
    return GetEnemyHurtboxCenter(enemy);
}

D3DXVECTOR3 GetJiraiyaAtEnemyPos(const Fighter& enemy) {
    const AABB& hb = enemy.GetHurtbox();
    return D3DXVECTOR3(
        hb.x + hb.width * 0.5f,
        CHARACTER_GROUND_Y,
        0.0f);
}

D3DXVECTOR3 GetJiraiyaBehindYosukePos(const D3DXVECTOR3& yosukePos, int yosukeFacing) {
    return D3DXVECTOR3(
        yosukePos.x - (float)yosukeFacing * kJiraiyaBehindX,
        yosukePos.y - kJiraiyaBehindY,
        0.0f);
}

D3DXVECTOR3 GetMirageSlashJiraiyaPos(const Fighter& enemy, int yosukeFacing) {
    const AABB& hb = enemy.GetHurtbox();
    const float scale = GetMakotoDrawScale();
    const float frontEdge = (yosukeFacing > 0) ? hb.x : (hb.x + hb.width);
    return D3DXVECTOR3(
        frontEdge - (float)yosukeFacing * kMirageFrontOffset * scale,
        CHARACTER_GROUND_Y,
        0.0f);
}

D3DXVECTOR3 GetJiraiyaAtEnemyFrontPos(const Fighter& enemy, int yosukeFacing) {
    return GetMirageSlashJiraiyaPos(enemy, yosukeFacing);
}

const YosukeTexture* GetTextureForState(int state) {
    switch (state) {
    case YOSUKE_INTRO: return &g_Intro;
    case YOSUKE_STANCE:
    case YOSUKE_IDLE: return &g_Stance;
    case YOSUKE_WALK: return &g_Walk;
    case YOSUKE_RUN: return &g_Run;
    case YOSUKE_DASH: return &g_Dash;
    case YOSUKE_BACK_DASH: return &g_BackDash;
    case YOSUKE_JUMP: return &g_Jump;
    case YOSUKE_GUARD: return &g_Guard;
    case YOSUKE_GUARD_AIR: return g_GuardAir.texture ? &g_GuardAir : &g_Guard;
    case YOSUKE_ATTACK: return &g_Attack;
    case YOSUKE_AIR_COMBO: return &g_AirCombo;
    case YOSUKE_CRESCENT_SLASH: return &g_CrescentSlash;
    case YOSUKE_MOONSAULT: return &g_Moonsault;
    case YOSUKE_FLYING_KUNAI: return &g_FlyingKunai;
    case YOSUKE_PERSONA_SUMMON:
    case YOSUKE_PERSONA_JIRAIYA: return &g_PersonaAttack;
    case YOSUKE_MIRAGE_SLASH: return &g_MirageSlash;
    case YOSUKE_BRAVE_BLADE: return &g_BraveBlade;
    case YOSUKE_GARUDYNE: return &g_Garudyne;
    case YOSUKE_DAMAGE: return &g_Damage;
    case YOSUKE_RECOVER: return &g_Recover;
    case YOSUKE_WIN: return &g_Win;
    case YOSUKE_LOSE: return &g_Damage;
    default: return nullptr;
    }
}

int GetMaxFrameForState(int state) {
    const YosukeTexture* tex = GetTextureForState(state);
    return tex ? tex->maxFrame : 1;
}

const YosukeTexture* GetJiraiyaTextureForState(int state) {
    switch (state) {
    case YOSUKE_PERSONA_JIRAIYA: return &g_JiraiyaAttack;
    case YOSUKE_MIRAGE_SLASH: return &g_JiraiyaMirageSlash;
    case YOSUKE_BRAVE_BLADE: return &g_JiraiyaBraveBlade;
    case YOSUKE_GARUDYNE: return &g_JiraiyaGarudyne;
    case YOSUKE_WIN: return &g_JiraiyaWin;
    default: return nullptr;
    }
}

bool AllowsMovement(int state) {
    switch (state) {
    case YOSUKE_STANCE:
    case YOSUKE_WALK:
    case YOSUKE_RUN:
    case YOSUKE_GUARD:
    case YOSUKE_GUARD_AIR:
        return true;
    default:
        return false;
    }
}

bool ResetsAnimationOnEnter(int state) {
    switch (state) {
    case YOSUKE_STANCE:
    case YOSUKE_WALK:
    case YOSUKE_RUN:
    case YOSUKE_GUARD:
    case YOSUKE_GUARD_AIR:
        return false;
    default:
        return true;
    }
}

bool IsMeleeState(int state) {
    switch (state) {
    case YOSUKE_ATTACK:
    case YOSUKE_AIR_COMBO:
    case YOSUKE_CRESCENT_SLASH:
    case YOSUKE_MOONSAULT:
    case YOSUKE_DASH:
        return true;
    default:
        return false;
    }
}

bool IsSkillState(int state) {
    return state == YOSUKE_FLYING_KUNAI ||
        state == YOSUKE_PERSONA_SUMMON ||
        state == YOSUKE_PERSONA_JIRAIYA ||
        state == YOSUKE_MIRAGE_SLASH ||
        state == YOSUKE_BRAVE_BLADE ||
        state == YOSUKE_GARUDYNE;
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

float GetYosukeBodyFeetY(int state, int frameIndex, bool groundedKnockdown = false) {
    if (state == YOSUKE_WALK || state == YOSUKE_RUN) {
        return YOSUKE_RUN_FEET_Y;
    }
    if (state == YOSUKE_DAMAGE || state == YOSUKE_LOSE) {
        if (groundedKnockdown) {
            return GetGroundedDamageDrawFeetY(
                YOSUKE_DAMAGE_FEET_Y,
                (int)(sizeof(YOSUKE_DAMAGE_FEET_Y) / sizeof(YOSUKE_DAMAGE_FEET_Y[0])),
                frameIndex,
                YOSUKE_KNOCKDOWN_GROUND_FEET_Y);
        }
        return SampleFeetYTable(
            YOSUKE_DAMAGE_FEET_Y,
            (int)(sizeof(YOSUKE_DAMAGE_FEET_Y) / sizeof(YOSUKE_DAMAGE_FEET_Y[0])),
            frameIndex);
    }
    if (state == YOSUKE_RECOVER) {
        if (groundedKnockdown) {
            return GetGroundedRecoverDrawFeetY(
                YOSUKE_RECOVER_FEET_Y_TABLE,
                (int)(sizeof(YOSUKE_RECOVER_FEET_Y_TABLE) / sizeof(YOSUKE_RECOVER_FEET_Y_TABLE[0])),
                frameIndex);
        }
        return SampleFeetYTable(
            YOSUKE_RECOVER_FEET_Y_TABLE,
            (int)(sizeof(YOSUKE_RECOVER_FEET_Y_TABLE) / sizeof(YOSUKE_RECOVER_FEET_Y_TABLE[0])),
            frameIndex);
    }
    return YOSUKE_STANCE_FEET_Y;
}

void DrawLayer(
    LPD3DXSPRITE sprite,
    const YosukeTexture& tex,
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

void DrawCenteredLayer(
    LPD3DXSPRITE sprite,
    const YosukeTexture& tex,
    int frameIndex,
    const D3DXVECTOR3& centerPos,
    float scale,
    D3DCOLOR color)
{
    if (!sprite || !tex.texture) return;
    SetFrameRect(g_SrcRect, tex, frameIndex);
    DrawScaledCharacterSprite(
        sprite,
        tex.texture,
        &g_SrcRect,
        centerPos,
        1,
        scale,
        color,
        MAKOTO_BODY_HEIGHT,
        YOSUKE_STANCE_FEET_Y);
}

void DrawSkillEffectLayer(
    LPD3DXSPRITE sprite,
    const YosukeTexture& tex,
    int frameIndex,
    const D3DXVECTOR3& feetPos,
    int facingDirection,
    D3DCOLOR color)
{
    if (!sprite || !tex.texture) return;
    SetFrameRect(g_SrcRect, tex, frameIndex);
    DrawScaledCharacterSprite(
        sprite,
        tex.texture,
        &g_SrcRect,
        feetPos,
        facingDirection,
        AGI_EFFECT_SCALE * PERSONA_EFFECT_SCALE,
        color,
        (float)kCellSize,
        YOSUKE_STANCE_FEET_Y);
}

bool LoadSheet(YosukeTexture& tex, const char* path, int frameCount) {
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
        D3DCOLOR_XRGB(YOSUKE_COLORKEY_R, YOSUKE_COLORKEY_G, YOSUKE_COLORKEY_B),
        NULL,
        NULL,
        &tex.texture);

    if (FAILED(hr) || !tex.texture) {
        return false;
    }

    ApplyYosukeColorKey(tex.texture);

    D3DSURFACE_DESC desc;
    tex.texture->GetLevelDesc(0, &desc);
    tex.cols = (int)(desc.Width / kCellSize);
    tex.rows = (int)(desc.Height / kCellSize);
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

void ReleaseSheet(YosukeTexture& tex) {
    if (tex.texture) {
        tex.texture->Release();
        tex.texture = nullptr;
    }
    tex.cols = 1;
    tex.rows = 1;
    tex.maxFrame = 1;
}

} // namespace

Yosuke::Yosuke()
    : currentFrame(0)
    , maxFrame(1)
    , animAccumulator(0)
    , currentState(YOSUKE_INTRO)
    , jumpCount(0)
    , jumpHorizontalSpeed(0.0f)
    , verticalVelocity(0.0f)
    , hitThisAttack(false)
    , dashHasHit(false)
    , attackButtonHeld(false)
    , skillHit(false)
    , jumpSpaceWasReleased(true)
    , personaAnimAccumulator(0)
    , jiraiyaFrame(0)
    , effectFrame(0)
    , showJiraiya(false)
    , showEffect(false)
    , jiraiyaPos(0.0f, 0.0f, 0.0f)
    , effectPos(0.0f, 0.0f, 0.0f)
    , noInputFrames(0)
    , idleWaitFrames(0)
    , damageGroundHold(0)
    , introDisplayHold(0)
    , introLastFrame(0)
    , spaceChordBuffer(0)
    , spaceWasDown(false)
    , crescentButtonHeld(false)
{
    characterId = Char_Yosuke;
    maxHealth = YOSUKE_MAX_HEALTH;
    health = YOSUKE_MAX_HEALTH;
    velocity = YOSUKE_MOVE_SPEED;
    ApplySlotSpawnDefaults();
    position.y = YOSUKE_INTRO_DROP_START_Y;
    maxFrame = GetMaxFrameForState(YOSUKE_INTRO);
    UpdateScaledHurtbox();
}

AABB Yosuke::GetBodyCollisionBox() const {
    AABB box = MakeLivePushbox(
        position,
        facingDirection,
        MAKOTO_BODY_WIDTH,
        MAKOTO_BODY_HEIGHT,
        GetMakotoDrawScale());

    if (currentState == YOSUKE_DASH) {
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

void Yosuke::UpdateScaledHurtbox() {
    hurtbox = GetBodyCollisionBox();
}

bool Yosuke::IsOnGround() const {
    return position.y >= CHARACTER_GROUND_Y - GROUND_CONTACT_EPSILON;
}

void Yosuke::ApplyGravity(int steps) {
    ApplyPhysicsGravitySteps(steps, verticalVelocity);
}

bool Yosuke::IsSuperMoveActive() const {
    return currentState == YOSUKE_GARUDYNE;
}

D3DCOLOR Yosuke::GetOverlayColor() const {
    if (!IsSuperMoveActive()) return 0;
    return D3DCOLOR_ARGB(220, 0, 0, 0);
}

bool Yosuke::CanUseSpaceChord() const {
    return spaceChordBuffer > 0;
}

void Yosuke::TickSpaceChordBuffer(bool isJumpPressed, int steps) {
    if (isJumpPressed && !spaceWasDown) {
        spaceChordBuffer = 36;
    }
    else if (isJumpPressed && spaceChordBuffer > 0) {
        spaceChordBuffer = 36;
    }
    else if (spaceChordBuffer > 0) {
        spaceChordBuffer -= steps;
        if (spaceChordBuffer < 0) spaceChordBuffer = 0;
    }
    spaceWasDown = isJumpPressed;
}

bool Yosuke::IsHoldingAwayInput() const {
    return IsHoldingBackInput(*this);
}

void Yosuke::UpdateLiveSkillTargets(Fighter& enemy) {
    if (currentState == YOSUKE_FLYING_KUNAI ||
        currentState == YOSUKE_PERSONA_JIRAIYA ||
        currentState == YOSUKE_BRAVE_BLADE) {
        effectPos = GetSkillEffectPos(enemy);
    }
    if (currentState == YOSUKE_PERSONA_JIRAIYA) {
        jiraiyaPos = GetJiraiyaAtEnemyFrontPos(enemy, facingDirection);
    }
    else if (currentState == YOSUKE_BRAVE_BLADE) {
        jiraiyaPos = GetJiraiyaAtEnemyPos(enemy);
    }
    else if (currentState == YOSUKE_GARUDYNE) {
        jiraiyaPos = GetJiraiyaBehindYosukePos(position, facingDirection);
    }
    else if (currentState == YOSUKE_MIRAGE_SLASH) {
        jiraiyaPos = GetMirageSlashJiraiyaPos(enemy, facingDirection);
    }
}

void Yosuke::EnterState(int state) {
    if (ResetsAnimationOnEnter(state)) {
        currentFrame = 0;
        animAccumulator = 0;
        hitThisAttack = false;
    }
    currentState = state;
    maxFrame = GetMaxFrameForState(state);
    if (maxFrame < 1) maxFrame = 1;
}

void Yosuke::CompleteToStance() {
    showJiraiya = false;
    showEffect = false;
    skillHit = false;
    personaAnimAccumulator = 0;
    jiraiyaFrame = 0;
    effectFrame = 0;
    hitThisAttack = false;
    dashHasHit = false;
    jumpCount = 0;
    verticalVelocity = 0.0f;
    if (IsOnGround()) {
        position.y = CHARACTER_GROUND_Y;
    }
    EnterState(YOSUKE_STANCE);
    idleWaitFrames = 0;
    noInputFrames = 0;
}

void Yosuke::BeginPersonaSummon() {
    g_SoundManager.PlayPersonaSummonSfx();
    showJiraiya = false;
    showEffect = false;
    jiraiyaFrame = 0;
    effectFrame = 0;
    skillHit = false;
    if (Fighter* foe = GetOpponent(*this)) {
        PullEnemyForUltimate(*this, *foe, true);
    }
    EnterState(YOSUKE_PERSONA_SUMMON);
}

void Yosuke::BeginGarudyne() {
    g_SoundManager.PlayPersonaSummonSfx();
    EnterState(YOSUKE_GARUDYNE);
    skillHit = false;
    skillEndHold = 0;
    showJiraiya = true;
    showEffect = false;
    jiraiyaFrame = 0;
    effectFrame = 0;
    personaAnimAccumulator = 0;
    jiraiyaPos = GetJiraiyaBehindYosukePos(position, facingDirection);
    if (Fighter* foe = GetOpponent(*this)) {
        PullEnemyForUltimate(*this, *foe, true);
    }
}

void Yosuke::UpdateIntroDropPosition() {
    const float t = (maxFrame > 1)
        ? ((float)currentFrame / (float)(maxFrame - 1))
        : 1.0f;
    position.y = YOSUKE_INTRO_DROP_START_Y +
        (CHARACTER_GROUND_Y - YOSUKE_INTRO_DROP_START_Y) * t;
}

void Yosuke::UpdateIntro(int steps) {
    maxFrame = GetMaxFrameForState(YOSUKE_INTRO);
    if (maxFrame < 1) {
        position.y = CHARACTER_GROUND_Y;
        CompleteToStance();
        return;
    }

    if (introDisplayHold > 0) {
        currentFrame = introLastFrame;
        introDisplayHold -= steps;
        UpdateIntroDropPosition();
        if (introDisplayHold <= 0) {
            position.y = CHARACTER_GROUND_Y;
            CompleteToStance();
        }
        return;
    }

    if (AdvanceOneShotFrame(animAccumulator, currentFrame, steps, kIntroTicks, maxFrame)) {
        introLastFrame = (maxFrame > 0) ? (maxFrame - 1) : 0;
        currentFrame = introLastFrame;
        introDisplayHold = kIntroEndHoldFrames;
    }
    UpdateIntroDropPosition();
}

void Yosuke::BeginHitReaction() {
    isHit = true;
    hitStunTimer = kHitStunFrames;
    currentState = YOSUKE_DAMAGE;
    currentFrame = 0;
    animAccumulator = 0;
    damageGroundHold = 0;
    maxFrame = GetMaxFrameForState(YOSUKE_DAMAGE);
    if (maxFrame < 1) maxFrame = 1;
    showJiraiya = false;
    showEffect = false;
    skillHit = false;
    idleWaitFrames = 0;
    ApplyStandardHitReactionVertical(position, verticalVelocity, IsOnGround());
    if (IsOnGround()) {
        currentFrame = maxFrame - 1;
    }
}

void Yosuke::BeginRecover() {
    isHit = false;
    hitStunTimer = 0;
    currentState = YOSUKE_RECOVER;
    currentFrame = 0;
    animAccumulator = 0;
    maxFrame = GetMaxFrameForState(YOSUKE_RECOVER);
    if (!g_Recover.texture || maxFrame < 1) {
        FinishRecover();
        return;
    }
}

void Yosuke::FinishRecover() {
    verticalVelocity = 0.0f;
    jumpCount = 0;
    isHit = false;
    hitStunTimer = 0;
    CompleteToStance();
    UpdateScaledHurtbox();
}

bool YosukeMeleeBoxesOverlap(const Fighter& attacker, const Fighter& defender, const AABB& attackBox) {
    const AABB foeHurt = defender.GetHurtbox();
    if (CollisionHelper::AABBIntersect(attackBox, foeHurt)) {
        return true;
    }

    AABB reach = attacker.GetBodyCollisionBox();
    const float extra = 40.0f * GetMakotoDrawScale();
    if (attacker.GetFacingDirection() > 0) {
        reach.width += extra;
    }
    else {
        reach.x -= extra;
        reach.width += extra;
    }
    return CollisionHelper::AABBIntersect(reach, foeHurt);
}

bool YosukeIsCloseForSkillHit(const Fighter& attacker, const Fighter& defender) {
    const float dx = fabsf(defender.GetHurtbox().x + defender.GetHurtbox().width * 0.5f - attacker.position.x);
    return dx <= kYosukeSkillProximityX * GetMakotoDrawScale();
}

void Yosuke::CheckAttackCollision(Fighter& enemy) {
    if (!enemy.CanReceiveHit()) return;

    const float scale = GetCharacterRenderScale();
    AttackData* data = nullptr;
    int specialDamage = 0;

    switch (currentState) {
    case YOSUKE_ATTACK:
    case YOSUKE_AIR_COMBO:
        data = &attackHitbox;
        break;
    case YOSUKE_CRESCENT_SLASH:
        data = &attackUpHitbox;
        specialDamage = kCrescentSlashDamage;
        break;
    case YOSUKE_MOONSAULT:
        data = &attackHitbox;
        specialDamage = kMoonsaultDamage;
        break;
    case YOSUKE_DASH:
        break;
    default:
        return;
    }

    if (currentState == YOSUKE_DASH) {
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
            if (CollisionHelper::AABBIntersect(dashBox, enemy.GetHurtbox()) && enemy.CanReceiveHit()) {
                DealMeleeHit(*this, enemy, DASH_HIT_DAMAGE);
                dashHasHit = true;
                RestoreSp(SP_GAIN_ON_HIT);
            }
        }
        return;
    }

    if (!data) return;
    if (currentFrame == 0) hitThisAttack = false;

    const int totalFrames = GetMaxFrameForState(currentState);
    int startF = (data->startFrame > 0) ? data->startFrame : max(1, totalFrames / 4);
    int endF = min(totalFrames - 1, data->endFrame);
    if (currentState == YOSUKE_ATTACK) {
        startF = 1;
        endF = min(totalFrames - 1, max(endF, 12));
    }

    if (currentFrame >= startF && currentFrame <= endF) {
        float attackX = 0.0f;
        float attackY = 0.0f;
        float boxW = 0.0f;
        float boxH = 0.0f;
        BuildAttackBox(position, facingDirection, scale, *data, attackX, attackY, boxW, boxH);
        if (facingDirection > 0) {
            boxW += kYosukeMeleeReachBonus;
        }
        else {
            attackX -= kYosukeMeleeReachBonus;
            boxW += kYosukeMeleeReachBonus;
        }

        const AABB attackBox = { attackX, attackY, boxW, boxH };
        if (!hitThisAttack && enemy.CanReceiveHit() &&
            YosukeMeleeBoxesOverlap(*this, enemy, attackBox)) {
            const int dmg = (specialDamage > 0) ? specialDamage : data->damage;
            DealMeleeHit(*this, enemy, dmg);
            hitThisAttack = true;
            RestoreSp(SP_GAIN_ON_HIT);
        }
    }
}

void Yosuke::UpdateGarudyne(int steps, Fighter& enemy) {
    PullEnemyForUltimate(*this, enemy, !skillHit && !enemy.IsHit());

    const YosukeTexture& bodyTex = g_Garudyne;
    const YosukeTexture& jiraiyaTex = g_JiraiyaGarudyne;
    maxFrame = bodyTex.maxFrame;
    if (maxFrame < 1) maxFrame = 1;

    // Slide horizontally toward the foe — stay on the ground (no climbing onto their head).
    const float targetX = GetEnemyHurtboxCenter(enemy).x;
    const float dx = targetX - position.x;
    if (fabsf(dx) > kGarudyneArriveRadius) {
        const float move = kGarudyneSpinSpeed * (float)steps;
        const float step = (move >= fabsf(dx)) ? fabsf(dx) : move;
        position.x += (dx > 0.0f) ? step : -step;
    }
    position.y = CHARACTER_GROUND_Y;
    verticalVelocity = 0.0f;

    jiraiyaPos = GetJiraiyaBehindYosukePos(position, facingDirection);
    UpdateScaledHurtbox();
    ClampFighterAgainstOpponent(*this, enemy);

    bool bodyDone = false;
    if (currentFrame < maxFrame - 1) {
        bodyDone = AdvanceOneShotFrame(animAccumulator, currentFrame, steps, kGarudyneSummonTicks, maxFrame);
    }
    else {
        bodyDone = true;
        currentFrame = maxFrame - 1;
    }

    personaAnimAccumulator += steps;
    while (personaAnimAccumulator >= kGarudynePersonaTicks) {
        personaAnimAccumulator -= kGarudynePersonaTicks;
        if (showJiraiya && jiraiyaTex.maxFrame > 0 && jiraiyaFrame < jiraiyaTex.maxFrame - 1) {
            jiraiyaFrame++;
        }
    }

    const bool jiraiyaDone = !showJiraiya || jiraiyaTex.maxFrame < 1 ||
        jiraiyaFrame >= jiraiyaTex.maxFrame - 1;

    if (!skillHit) {
        const bool hitReady = jiraiyaFrame >= kSkillHitStartFrame ||
            currentFrame >= max(1, maxFrame / 4);
        if (hitReady && enemy.CanReceiveHit()) {
            const AABB bodyBox = GetBodyCollisionBox();
            const AABB enemyBox = enemy.GetHurtbox();
            if (CollisionHelper::AABBIntersect(bodyBox, enemyBox) ||
                YosukeIsCloseForSkillHit(*this, enemy)) {
                DealSkillHit(*this, enemy, kGarudyneDamage);
                skillHit = true;
            }
        }
    }

    if (bodyDone && jiraiyaDone) {
        CompleteToStance();
    }
}

void Yosuke::UpdateSandbag(int steps) {
    const int animSteps = 1;

    if (currentState == YOSUKE_INTRO) {
        UpdateIntro(animSteps);
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == YOSUKE_DAMAGE) {

        ApplyGravity(steps);
        PinFighterToGround(position, verticalVelocity);
        maxFrame = GetMaxFrameForState(YOSUKE_DAMAGE);
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

    if (currentState == YOSUKE_RECOVER) {
        maxFrame = GetMaxFrameForState(YOSUKE_RECOVER);
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

    if (currentState == YOSUKE_IDLE) {
        maxFrame = GetMaxFrameForState(YOSUKE_IDLE);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, kIdlePlayTicks, maxFrame)) {
            currentState = YOSUKE_STANCE;
            currentFrame = 0;
            animAccumulator = 0;
            idleWaitFrames = 0;
            maxFrame = GetMaxFrameForState(YOSUKE_STANCE);
            if (maxFrame < 1) maxFrame = 1;
        }
        UpdateScaledHurtbox();
        return;
    }

    currentState = YOSUKE_STANCE;
    maxFrame = GetMaxFrameForState(YOSUKE_STANCE);
    if (maxFrame < 1) maxFrame = 1;
    AdvanceLoopFrame(animAccumulator, currentFrame, animSteps, kLoopSlowTicks, maxFrame);
    idleWaitFrames += steps;
    if (idleWaitFrames >= kIdleWaitFrames) {
        idleWaitFrames = 0;
        currentState = YOSUKE_IDLE;
        currentFrame = 0;
        animAccumulator = 0;
        maxFrame = GetMaxFrameForState(YOSUKE_IDLE);
        if (maxFrame < 1) maxFrame = 1;
    }
    UpdateScaledHurtbox();
}

void Yosuke::UpdateHuman(int steps) {
    Fighter* opponent = GetOpponent(*this);
    if (!opponent) return;

    const int animSteps = 1;
    const bool attackDownNow = IsGameMouseDown(VK_LBUTTON);
    const bool attackJustPressed = attackDownNow && !attackButtonHeld;
    attackButtonHeld = attackDownNow;

    if (currentState == YOSUKE_GARUDYNE) {
        UpdateGarudyne(steps, *opponent);
        UpdateScaledHurtbox();
        return;
    }

    bool isRunning = IsGameKeyDown(DIK_LSHIFT) || IsGameKeyDown(DIK_RSHIFT);
    const HorizontalMoveInput move = ReadHorizontalMoveInput();
    bool isMoving = move.isMoving;
    float moveDirX = move.moveDirX;

    const bool opponentOnRight = opponent->GetPosition().x >= position.x;

    if (move.leftHeld && !move.rightHeld) {
        facingDirection = -1;
    }
    else if (move.rightHeld && !move.leftHeld) {
        facingDirection = 1;
    }

    if (!isMoving) {
        facingDirection = opponentOnRight ? 1 : -1;
    }

    if (isRunning && isMoving) {
        if (!DrainStaminaWhileRunning(animSteps)) {
            isRunning = false;
        }
    }
    else {
        RegenStamina(animSteps);
    }
    const float currentVelocity = ComputeSmoothedLocomotionSpeed(
        velocity, isRunning && isMoving, runBlend, steps);
    const bool useRunAnim = ShouldUseRunLocomotion(runBlend);

    if (AllowsMovement(currentState) && isMoving &&
        currentState != YOSUKE_DASH && currentState != YOSUKE_BACK_DASH) {
        TryApplyHorizontalDelta(moveDirX * currentVelocity * steps);
        ClampMakotoCenterX(position.x);
    }

    if (AllowsMovement(currentState) && IsOnGround()) {
        position.y = CHARACTER_GROUND_Y;
        verticalVelocity = 0.0f;
    }

    if (currentState == YOSUKE_INTRO) {
        UpdateIntro(animSteps);
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == YOSUKE_PERSONA_SUMMON) {
        maxFrame = GetMaxFrameForState(YOSUKE_PERSONA_SUMMON);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, kSummonTicks, maxFrame)) {
            showJiraiya = true;
            showEffect = true;
            jiraiyaFrame = 0;
            effectFrame = 0;
            skillHit = false;
            if (Fighter* foe = GetOpponent(*this)) {
                PullEnemyForUltimate(*this, *foe, true);
                effectPos = GetSkillEffectPos(*foe);
                jiraiyaPos = GetJiraiyaAtEnemyFrontPos(*foe, facingDirection);
            }
            currentState = YOSUKE_PERSONA_JIRAIYA;
            currentFrame = (maxFrame > 0) ? (maxFrame - 1) : 0;
            animAccumulator = 0;
            maxFrame = g_JiraiyaAttack.maxFrame;
            if (maxFrame < 1) maxFrame = 1;
        }
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == YOSUKE_PERSONA_JIRAIYA) {
        maxFrame = g_JiraiyaAttack.maxFrame;
        if (maxFrame < 1) maxFrame = 1;
        PullEnemyForUltimate(*this, *opponent, !skillHit && !opponent->IsHit());
        UpdateLiveSkillTargets(*opponent);

        personaAnimAccumulator += animSteps;
        while (personaAnimAccumulator >= kPersonaEffectTicks) {
            personaAnimAccumulator -= kPersonaEffectTicks;
            if (jiraiyaFrame < maxFrame - 1) {
                jiraiyaFrame++;
            }
            if (showEffect && g_JiraiyaAttackEffect.maxFrame > 0 &&
                effectFrame < g_JiraiyaAttackEffect.maxFrame - 1) {
                effectFrame++;
            }
        }

        const bool jiraiyaDone = jiraiyaFrame >= maxFrame - 1;
        const bool effectDone = !showEffect || g_JiraiyaAttackEffect.maxFrame < 1 ||
            effectFrame >= g_JiraiyaAttackEffect.maxFrame - 1;

        if (!skillHit && (jiraiyaFrame >= kSkillHitStartFrame || effectFrame >= kSkillHitStartFrame)) {
            if (opponent->CanReceiveHit() && YosukeIsCloseForSkillHit(*this, *opponent)) {
                DealSkillHit(*this, *opponent, kPersonaStrikeDamage);
                skillHit = true;
            }
        }

        if (jiraiyaDone && effectDone) {
            CompleteToStance();
        }
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == YOSUKE_FLYING_KUNAI) {
        maxFrame = GetMaxFrameForState(YOSUKE_FLYING_KUNAI);
        showEffect = true;
        UpdateLiveSkillTargets(*opponent);

        personaAnimAccumulator += animSteps;
        while (personaAnimAccumulator >= kPersonaEffectTicks) {
            personaAnimAccumulator -= kPersonaEffectTicks;
            if (showEffect && g_FlyingKunaiEffect.maxFrame > 0 &&
                effectFrame < g_FlyingKunaiEffect.maxFrame - 1) {
                effectFrame++;
            }
        }

        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, kActionTicks, maxFrame)) {
            CompleteToStance();
        }
        else if (!skillHit && effectFrame >= kSkillHitStartFrame) {
            if (opponent->CanReceiveHit() && YosukeIsCloseForSkillHit(*this, *opponent)) {
                DealSkillHit(*this, *opponent, kFlyingKunaiDamage);
                skillHit = true;
            }
        }
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == YOSUKE_MIRAGE_SLASH) {
        showJiraiya = true;
        maxFrame = GetMaxFrameForState(currentState);
        PullEnemyForUltimate(*this, *opponent, !skillHit && !opponent->IsHit());
        UpdateLiveSkillTargets(*opponent);

        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, kSummonTicks, maxFrame)) {
            CompleteToStance();
        }
        else {
            jiraiyaFrame = currentFrame;
            if (jiraiyaFrame >= g_JiraiyaMirageSlash.maxFrame) {
                jiraiyaFrame = g_JiraiyaMirageSlash.maxFrame - 1;
            }
            if (!skillHit && currentFrame >= kSkillHitStartFrame) {
                if (opponent->CanReceiveHit() && YosukeIsCloseForSkillHit(*this, *opponent)) {
                    DealSkillHit(*this, *opponent, kMirageSlashDamage);
                    skillHit = true;
                }
            }
        }
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == YOSUKE_BRAVE_BLADE) {
        showJiraiya = true;
        maxFrame = GetMaxFrameForState(currentState);
        PullEnemyForUltimate(*this, *opponent, !skillHit && !opponent->IsHit());
        UpdateLiveSkillTargets(*opponent);

        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, kActionTicks, maxFrame)) {
            CompleteToStance();
        }
        else {
            personaAnimAccumulator += animSteps;
            while (personaAnimAccumulator >= kPersonaEffectTicks) {
                personaAnimAccumulator -= kPersonaEffectTicks;
                if (jiraiyaFrame < g_JiraiyaBraveBlade.maxFrame - 1) {
                    jiraiyaFrame++;
                }
            }
            if (!skillHit && (jiraiyaFrame >= kSkillHitStartFrame || currentFrame >= kSkillHitStartFrame)) {
                if (opponent->CanReceiveHit() && YosukeIsCloseForSkillHit(*this, *opponent)) {
                    DealSkillHit(*this, *opponent, kBraveBladeDamage);
                    skillHit = true;
                }
            }
        }
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == YOSUKE_IDLE) {
        const bool checkInput = isMoving ||
            IsGameKeyDown(DIK_SPACE) ||
            IsGameKeyDown(DIK_J) ||
            attackDownNow ||
            IsGameMouseDown(VK_RBUTTON) ||
            IsGameKeyDown(DIK_E) ||
            IsGameKeyDown(DIK_R) ||
            IsGameKeyDown(DIK_1) ||
            IsGameKeyDown(DIK_2) ||
            IsGameKeyDown(DIK_3) ||
            IsGameKeyDown(DIK_4);
        if (checkInput) {
            CompleteToStance();
            UpdateScaledHurtbox();
            return;
        }
        maxFrame = GetMaxFrameForState(YOSUKE_IDLE);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, kIdlePlayTicks, maxFrame)) {
            CompleteToStance();
        }
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == YOSUKE_DASH) {
        TryApplyHorizontalDelta((float)facingDirection * (velocity * kDashSpeedMultiplier) * steps);
        ClampMakotoCenterX(position.x);
        maxFrame = GetMaxFrameForState(YOSUKE_DASH);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, kActionTicks, maxFrame)) {
            dashHasHit = false;
            CompleteToStance();
        }
        CheckAttackCollision(*opponent);
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == YOSUKE_BACK_DASH) {
        const int away = GetGuardAwayDirection(*this);
        TryApplyHorizontalDelta((float)away * velocity * DODGE_SLIDE_SPEED * steps);
        ClampMakotoCenterX(position.x);
        maxFrame = GetMaxFrameForState(YOSUKE_BACK_DASH);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, kActionTicks, maxFrame)) {
            CompleteToStance();
        }
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == YOSUKE_GUARD_AIR) {
        ApplyGravity(steps);
        maxFrame = GetMaxFrameForState(YOSUKE_GUARD_AIR);
        AdvanceLoopFrame(animAccumulator, currentFrame, animSteps, kLoopSlowTicks, maxFrame);

        if (!IsHoldingGuardInput(*this)) {
            if (IsOnGround()) {
                jumpCount = 0;
                verticalVelocity = 0.0f;
                position.y = CHARACTER_GROUND_Y;
                CompleteToStance();
            }
            else {
                EnterState(YOSUKE_JUMP);
            }
        }
        else if (IsOnGround() && verticalVelocity >= 0.0f) {
            jumpCount = 0;
            verticalVelocity = 0.0f;
            position.y = CHARACTER_GROUND_Y;
            EnterState(YOSUKE_GUARD);
        }

        ClampMakotoCenterX(position.x);
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == YOSUKE_JUMP) {
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

        if (isJumpPressed && isAttackPressed) {
            showJiraiya = false;
            EnterState(YOSUKE_AIR_COMBO);
            UpdateScaledHurtbox();
            return;
        }

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

        maxFrame = GetMaxFrameForState(YOSUKE_JUMP);
        AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, kActionTicks, maxFrame);

        if (IsOnGround() && verticalVelocity >= 0.0f) {
            jumpCount = 0;
            CompleteToStance();
        }
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == YOSUKE_CRESCENT_SLASH) {
        maxFrame = GetMaxFrameForState(YOSUKE_CRESCENT_SLASH);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, kCrescentSlashTicks, maxFrame)) {
            CompleteToStance();
        }
        CheckAttackCollision(*opponent);
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == YOSUKE_ATTACK ||
        currentState == YOSUKE_AIR_COMBO ||
        currentState == YOSUKE_MOONSAULT) {
        maxFrame = GetMaxFrameForState(currentState);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, kActionTicks, maxFrame)) {
            CompleteToStance();
        }
        CheckAttackCollision(*opponent);
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == YOSUKE_DAMAGE) {
        ApplyGravity(steps);
        PinFighterToGround(position, verticalVelocity);
        maxFrame = GetMaxFrameForState(YOSUKE_DAMAGE);
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

    if (currentState == YOSUKE_RECOVER) {
        maxFrame = GetMaxFrameForState(YOSUKE_RECOVER);
        PinFighterToGround(position, verticalVelocity);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, kRecoverAnimTicks, maxFrame)) {
            FinishRecover();
        }
        UpdateScaledHurtbox();
        return;
    }

    const bool isJumpPressed = IsGameKeyDown(DIK_SPACE);
    const bool isDashHeld = IsGameKeyDown(DIK_J);
    const bool isBackDashPressed = IsGameMouseDown(VK_RBUTTON) && IsHoldingAwayInput();
    const bool isGuardPressed = IsHoldingGuardInput(*this);
    const bool isMoonsaultPressed = IsGameKeyDown(DIK_E);
    const bool isCrescentDownNow = IsGameKeyDown(DIK_R);
    const bool isCrescentJustPressed = isCrescentDownNow && !crescentButtonHeld;
    crescentButtonHeld = isCrescentDownNow;
    const bool isAttackPressed = attackJustPressed;
    const bool isKey1 = IsGameKeyDown(DIK_1);
    const bool isKey2 = IsGameKeyDown(DIK_2);
    const bool isKey3 = IsGameKeyDown(DIK_3);
    const bool isKey4 = IsGameKeyDown(DIK_4);

    TickSpaceChordBuffer(isJumpPressed, steps);
    const bool canUseSpaceChord = CanUseSpaceChord() || isJumpPressed;
    const bool isFlyingKunaiPressed = isMoonsaultPressed && canUseSpaceChord;

    const bool hasAnyInput = isMoving || isJumpPressed || isDashHeld || attackDownNow ||
        isBackDashPressed || isGuardPressed || isMoonsaultPressed || isCrescentDownNow ||
        isKey1 || isKey2 || isKey3 || isKey4;

    if (hasAnyInput) noInputFrames = 0;
    else if (currentState == YOSUKE_STANCE) noInputFrames += steps;

    int nextState = YOSUKE_STANCE;

    if (isKey4 && TryConsumeSp(kSpCostGarudyne)) {
        BeginGarudyne();
        UpdateScaledHurtbox();
        return;
    }
    if (isKey1 && TryConsumeSp(kSpCostPersona)) {
        BeginPersonaSummon();
        UpdateScaledHurtbox();
        return;
    }
    if (isKey2 && TryConsumeSp(kSpCostMirage)) {
        g_SoundManager.PlayPersonaSummonSfx();
        showJiraiya = true;
        jiraiyaFrame = 0;
        personaAnimAccumulator = 0;
        skillHit = false;
        PullEnemyForUltimate(*this, *opponent, true);
        jiraiyaPos = GetMirageSlashJiraiyaPos(*opponent, facingDirection);
        EnterState(YOSUKE_MIRAGE_SLASH);
        UpdateScaledHurtbox();
        return;
    }
    if (isKey3 && TryConsumeSp(kSpCostBraveBlade)) {
        g_SoundManager.PlayPersonaSummonSfx();
        showJiraiya = true;
        jiraiyaFrame = 0;
        personaAnimAccumulator = 0;
        skillHit = false;
        PullEnemyForUltimate(*this, *opponent, true);
        if (opponent) {
            jiraiyaPos = GetJiraiyaAtEnemyPos(*opponent);
            effectPos = GetSkillEffectPos(*opponent);
        }
        EnterState(YOSUKE_BRAVE_BLADE);
        UpdateScaledHurtbox();
        return;
    }

    if (isFlyingKunaiPressed && TryConsumeStamina(STAMINA_COST_ACTION)) {
        nextState = YOSUKE_FLYING_KUNAI;
        showEffect = true;
        effectFrame = 0;
        personaAnimAccumulator = 0;
        skillHit = false;
        effectPos = GetSkillEffectPos(*opponent);
        spaceChordBuffer = 0;
    }
    else if (isMoonsaultPressed && TryConsumeStamina(STAMINA_COST_ACTION)) {
        nextState = YOSUKE_MOONSAULT;
        showJiraiya = false;
    }
    else if (isCrescentJustPressed && TryConsumeStamina(STAMINA_COST_ACTION)) {
        nextState = YOSUKE_CRESCENT_SLASH;
        showJiraiya = false;
    }
    else if (isDashHeld && TryConsumeStamina(STAMINA_COST_ACTION)) {
        nextState = YOSUKE_DASH;
        dashHasHit = false;
    }
    else if (isBackDashPressed && TryConsumeStamina(STAMINA_COST_ACTION)) {
        nextState = YOSUKE_BACK_DASH;
    }
    else if (isJumpPressed && IsOnGround()) {
        nextState = YOSUKE_JUMP;
        jumpCount = 1;
        jumpSpaceWasReleased = false;
        verticalVelocity = kJumpVelocity;
        jumpHorizontalSpeed = (moveDirX != 0.0f)
            ? (moveDirX * currentVelocity * kAirControlMultiplier)
            : 0.0f;
    }
    else if (isAttackPressed) {
        nextState = YOSUKE_ATTACK;
        showJiraiya = false;
    }
    else if (isGuardPressed) {
        nextState = IsOnGround() ? YOSUKE_GUARD : YOSUKE_GUARD_AIR;
    }
    else if (isMoving) {
        nextState = useRunAnim ? YOSUKE_RUN : YOSUKE_WALK;
    }
    else if (noInputFrames >= IDLE_THRESHOLD_FRAMES) {
        nextState = YOSUKE_IDLE;
        noInputFrames = 0;
    }
    else {
        nextState = YOSUKE_STANCE;
    }

    if (currentState != nextState) {
        EnterState(nextState);
        if (nextState == YOSUKE_IDLE && IsOnGround()) {
            position.y = CHARACTER_GROUND_Y;
        }
    }

    maxFrame = GetMaxFrameForState(currentState);
    switch (currentState) {
    case YOSUKE_STANCE:
        AdvanceLoopFrame(animAccumulator, currentFrame, animSteps, kLoopSlowTicks, maxFrame);
        break;
    case YOSUKE_WALK:
        if (maxFrame < 1) maxFrame = 1;
        AdvanceLoopFrame(animAccumulator, currentFrame, animSteps, kWalkAnimTicks, maxFrame);
        break;
    case YOSUKE_RUN:
        if (maxFrame < 1) maxFrame = 1;
        AdvanceLoopFrame(animAccumulator, currentFrame, animSteps, kRunAnimTicks, maxFrame);
        break;
    case YOSUKE_GUARD:
    case YOSUKE_GUARD_AIR:
        AdvanceLoopFrame(animAccumulator, currentFrame, animSteps, kLoopSlowTicks, maxFrame);
        break;
    default:
        break;
    }

    UpdateScaledHurtbox();
}

void Yosuke::SyncHeldInputState() {
    attackButtonHeld = IsGameMouseDown(VK_LBUTTON);
    crescentButtonHeld = IsGameKeyDown(DIK_R);
}

void Yosuke::Update() {
    if (IsPlayingResultPose()) {
        maxFrame = GetMaxFrameForState(currentState);
        if (maxFrame < 1) maxFrame = 1;

        int steps = g_GameTimer.GetLastFramesToUpdate();
        if (steps <= 0) steps = 1;
        if (steps > GAME_TIMER_MAX_STEPS_PER_FRAME) steps = GAME_TIMER_MAX_STEPS_PER_FRAME;

        if (currentState == YOSUKE_LOSE) {
            const int knockdownFrames = g_Damage.maxFrame;
            if (knockdownFrames > 0) {
                maxFrame = knockdownFrames;
            }
            currentFrame = maxFrame - 1;
        }
        else if (currentState == YOSUKE_WIN) {
            if (resultPoseHoldFrame < 0) {
                if (g_Win.texture) {
                    resultPoseHoldFrame = FindLastVisibleSheetFrame(
                        g_Win.texture,
                        kCellSize,
                        kCellSize,
                        g_Win.cols,
                        maxFrame);
                }
                else {
                    resultPoseHoldFrame = maxFrame - 1;
                }
            }

            const int winEndFrame = resultPoseHoldFrame;
            const int winAnimFrames = winEndFrame + 1;
            if (!resultPoseAnimLocked) {
                if (currentFrame < winEndFrame) {
                    if (AdvanceOneShotFrame(
                        animAccumulator,
                        currentFrame,
                        steps,
                        BATTLE_WIN_ANIM_TICKS,
                        winAnimFrames)) {
                        currentFrame = winEndFrame;
                        resultPoseAnimLocked = true;
                    }
                }
                else {
                    currentFrame = winEndFrame;
                    resultPoseAnimLocked = true;
                }
            }
            else {
                currentFrame = winEndFrame;
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

void Yosuke::RenderSkillBackdropBeforeOpponent(LPD3DXSPRITE sprite) {
    if (!sprite || (isDead && !IsPlayingResultPose() && !IsBattleEndSequence())) return;

    Fighter* opponent = GetOpponent(*this);
    if (opponent) {
        UpdateLiveSkillTargets(*opponent);
    }

    const D3DCOLOR color = ApplySpriteTint(D3DCOLOR_XRGB(255, 255, 255), GetSpriteTint());

    if (showEffect && currentState == YOSUKE_FLYING_KUNAI && g_FlyingKunaiEffect.texture) {
        DrawCenteredLayer(sprite, g_FlyingKunaiEffect, effectFrame, effectPos,
            PERSONA_EFFECT_SCALE, color);
        return;
    }

    if (showEffect && currentState == YOSUKE_PERSONA_JIRAIYA && g_JiraiyaAttackEffect.texture) {
        DrawSkillEffectLayer(sprite, g_JiraiyaAttackEffect, effectFrame, effectPos, facingDirection, color);
    }
}

void Yosuke::Render(LPD3DXSPRITE sprite) {
    if (!sprite || (isDead && !IsPlayingResultPose() && !IsBattleEndSequence())) return;

    const D3DCOLOR color = ApplySpriteTint(D3DCOLOR_XRGB(255, 255, 255), GetSpriteTint());
    const bool groundedKnockdown =
        IsFighterAtGroundLevel(position) &&
        (currentState == YOSUKE_DAMAGE || currentState == YOSUKE_RECOVER || currentState == YOSUKE_LOSE);
    float bodyFeetY = GetYosukeBodyFeetY(currentState, currentFrame, groundedKnockdown);
    const YosukeTexture* knockdownTex = GetTextureForState(currentState);
    if (knockdownTex && knockdownTex->texture &&
        (currentState == YOSUKE_DAMAGE || currentState == YOSUKE_RECOVER || currentState == YOSUKE_LOSE)) {
        bodyFeetY = MeasureTextureFrameBottomY(
            knockdownTex->texture,
            currentFrame,
            kCellSize,
            kCellSize,
            knockdownTex->cols);
    }

    const YosukeTexture* jiraiyaTex = GetJiraiyaTextureForState(currentState);
    if (showJiraiya && jiraiyaTex && jiraiyaTex->texture && jiraiyaTex->maxFrame > 0) {
        D3DXVECTOR3 pos;
        if (currentState == YOSUKE_PERSONA_JIRAIYA) {
            if (Fighter* opponent = GetOpponent(*this)) {
                pos = GetJiraiyaAtEnemyFrontPos(*opponent, facingDirection);
            }
            else {
                pos = jiraiyaPos;
            }
        }
        else if (currentState == YOSUKE_BRAVE_BLADE) {
            if (Fighter* opponent = GetOpponent(*this)) {
                pos = GetJiraiyaAtEnemyPos(*opponent);
            }
            else {
                pos = jiraiyaPos;
            }
        }
        else if (currentState == YOSUKE_GARUDYNE) {
            pos = GetJiraiyaBehindYosukePos(position, facingDirection);
        }
        else if (currentState == YOSUKE_MIRAGE_SLASH) {
            if (Fighter* opponent = GetOpponent(*this)) {
                pos = GetMirageSlashJiraiyaPos(*opponent, facingDirection);
            }
            else {
                pos = jiraiyaPos;
            }
        }
        else {
            pos = D3DXVECTOR3(
                position.x - (float)facingDirection * kJiraiyaBehindX,
                position.y - kJiraiyaBehindY,
                0.0f);
        }
        int frame = (currentState == YOSUKE_PERSONA_JIRAIYA ||
            currentState == YOSUKE_MIRAGE_SLASH ||
            currentState == YOSUKE_BRAVE_BLADE ||
            currentState == YOSUKE_GARUDYNE)
            ? jiraiyaFrame
            : currentFrame;
        if (frame >= jiraiyaTex->maxFrame) {
            frame = jiraiyaTex->maxFrame - 1;
        }
        DrawLayer(sprite, *jiraiyaTex, frame, pos, facingDirection, kJiraiyaDrawScale, color, YOSUKE_STANCE_FEET_Y);
    }

    if (currentState == YOSUKE_WIN && g_JiraiyaWin.texture && g_JiraiyaWin.maxFrame > 0) {
        const D3DXVECTOR3 winJiraiyaPos(
            position.x - (float)facingDirection * kJiraiyaBehindX,
            position.y - kJiraiyaBehindY,
            0.0f);
        int winFrame = (resultPoseAnimLocked && resultPoseHoldFrame >= 0)
            ? resultPoseHoldFrame
            : currentFrame;
        if (winFrame >= g_JiraiyaWin.maxFrame) {
            winFrame = g_JiraiyaWin.maxFrame - 1;
        }
        DrawLayer(sprite, g_JiraiyaWin, winFrame, winJiraiyaPos, facingDirection,
            kJiraiyaDrawScale, color, YOSUKE_STANCE_FEET_Y);
    }

    const YosukeTexture* bodyTex = GetTextureForState(currentState);
    if (bodyTex && bodyTex->texture) {
        int bodyFrame = currentFrame;
        if (currentState == YOSUKE_PERSONA_JIRAIYA && g_PersonaAttack.maxFrame > 0) {
            bodyFrame = g_PersonaAttack.maxFrame - 1;
        }
        if (bodyFrame >= bodyTex->maxFrame) {
            bodyFrame = bodyTex->maxFrame - 1;
        }
        if (bodyFrame < 0) {
            bodyFrame = 0;
        }
        DrawLayer(sprite, *bodyTex, bodyFrame, position, facingDirection, 1.0f, color, bodyFeetY);
    }
}

void Yosuke::RenderDebugHitbox(LPD3DXSPRITE sprite) {
    if (!sprite) return;
    UpdateScaledHurtbox();
    const AABB bodyBox = GetBodyCollisionBox();
    DrawDebugRect(sprite, bodyBox.x, bodyBox.y, bodyBox.width, bodyBox.height, D3DCOLOR_ARGB(160, 255, 64, 255));
    DrawDebugRect(sprite, hurtbox.x, hurtbox.y, hurtbox.width, hurtbox.height, D3DCOLOR_ARGB(160, 80, 200, 120));

    if (currentState == YOSUKE_GARUDYNE) {
        DrawDebugRect(sprite, bodyBox.x, bodyBox.y, bodyBox.width, bodyBox.height,
            D3DCOLOR_ARGB(140, 255, 180, 64));
        return;
    }

    if (!IsMeleeState(currentState)) return;

    Fighter* opponent = GetOpponent(*this);
    if (!opponent) return;

    AttackData* data = nullptr;
    switch (currentState) {
    case YOSUKE_ATTACK:
    case YOSUKE_AIR_COMBO:
    case YOSUKE_MOONSAULT: data = &attackHitbox; break;
    case YOSUKE_CRESCENT_SLASH: data = &attackUpHitbox; break;
    default: break;
    }
    if (!data) return;

    float attackX = 0.0f;
    float attackY = 0.0f;
    float boxW = 0.0f;
    float boxH = 0.0f;
    BuildAttackBox(position, facingDirection, GetCharacterRenderScale(), *data, attackX, attackY, boxW, boxH);
    if (facingDirection > 0) {
        boxW += kYosukeMeleeReachBonus;
    }
    else {
        attackX -= kYosukeMeleeReachBonus;
        boxW += kYosukeMeleeReachBonus;
    }
    DrawDebugRect(sprite, attackX, attackY, boxW, boxH, D3DCOLOR_ARGB(140, 255, 64, 64));
}

void Yosuke::TakeDamage(int damage) {
    if (isDead) return;

    int appliedDamage = damage;
    if (TryProcessGuardBlock(*this, damage, appliedDamage)) {
        if (appliedDamage > 0) {
            health -= appliedDamage;
            if (health < 0) health = 0;
        }
        NotifyFighterDamageApplied(*this, appliedDamage);
        UpdateScaledHurtbox();
        return;
    }

    if (!IsHumanControlled()) {
        health -= appliedDamage;
        if (health < 0) health = 0;
        if (currentState != YOSUKE_DAMAGE && currentState != YOSUKE_RECOVER) {
            BeginHitReaction();
        }
        if (!TRAINING_MODE && ShouldFighterDieOnZeroHealth() && health <= 0) {
            isDead = true;
        }
        NotifyFighterDamageApplied(*this, appliedDamage);
        return;
    }

    health -= appliedDamage;
    if (health < 0) health = 0;
    if (currentState != YOSUKE_DAMAGE && currentState != YOSUKE_RECOVER &&
        currentState != YOSUKE_GARUDYNE) {
        BeginHitReaction();
    }
    else {
        isHit = true;
        hitStunTimer = kHitStunFrames;
    }

    if (!TRAINING_MODE && ShouldFighterDieOnZeroHealth() && health <= 0) {
        isDead = true;
    }

    NotifyFighterDamageApplied(*this, appliedDamage);
}

bool Yosuke::IsInGuardState() const {
    return currentState == YOSUKE_GUARD || currentState == YOSUKE_GUARD_AIR;
}

void Yosuke::HoldGuardState(bool airborne) {
    const int target = airborne ? YOSUKE_GUARD_AIR : YOSUKE_GUARD;
    if (currentState == target) {
        return;
    }
    EnterState(target);
}

void Yosuke::ApplySkillDamage(int damage) {
    if (isDead) return;
    const int appliedDamage = damage;
    health -= appliedDamage;
    if (health < 0) health = 0;

    if (currentState == YOSUKE_DAMAGE || currentState == YOSUKE_RECOVER) {
        isHit = true;
        hitStunTimer = kHitStunFrames;
    }
    else if (currentState != YOSUKE_GARUDYNE) {
        BeginHitReaction();
    }

    if (!TRAINING_MODE && ShouldFighterDieOnZeroHealth() && health <= 0) {
        isDead = true;
    }

    NotifyFighterDamageApplied(*this, appliedDamage);
}

void Yosuke::BeginVictoryPose() {
    isHit = false;
    hitStunTimer = 0;
    resultPoseAnimLocked = false;
    resultPoseHoldFrame = -1;
    showJiraiya = false;
    showEffect = false;
    currentState = YOSUKE_WIN;
    currentFrame = 0;
    animAccumulator = 0;
    maxFrame = GetMaxFrameForState(YOSUKE_WIN);
    if (maxFrame < 1) maxFrame = 1;
    resultPoseHoldFrame = g_Win.texture
        ? FindLastVisibleSheetFrame(
            g_Win.texture,
            kCellSize,
            kCellSize,
            g_Win.cols,
            maxFrame)
        : maxFrame - 1;
    position.y = CHARACTER_GROUND_Y;
    verticalVelocity = 0.0f;
    UpdateScaledHurtbox();
}

void Yosuke::BeginDefeatPose() {
    isHit = false;
    hitStunTimer = 0;
    resultPoseHoldFrame = -1;
    showJiraiya = false;
    showEffect = false;
    currentState = YOSUKE_LOSE;
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

bool Yosuke::IsPlayingResultPose() const {
    return currentState == YOSUKE_WIN || currentState == YOSUKE_LOSE;
}

bool Yosuke::IsInKnockdownReaction() const {
    return isHit || currentState == YOSUKE_DAMAGE || currentState == YOSUKE_RECOVER;
}

bool Yosuke::IsInCombatAction() const {
    switch (currentState) {
    case YOSUKE_ATTACK:
    case YOSUKE_AIR_COMBO:
    case YOSUKE_CRESCENT_SLASH:
    case YOSUKE_MOONSAULT:
    case YOSUKE_FLYING_KUNAI:
    case YOSUKE_DASH:
    case YOSUKE_BACK_DASH:
    case YOSUKE_JUMP:
    case YOSUKE_PERSONA_SUMMON:
    case YOSUKE_PERSONA_JIRAIYA:
    case YOSUKE_MIRAGE_SLASH:
    case YOSUKE_BRAVE_BLADE:
    case YOSUKE_GARUDYNE:
        return true;
    default:
        return false;
    }
}

void Yosuke::Reset() {
    ApplySlotSpawnDefaults();
    position.y = YOSUKE_INTRO_DROP_START_Y;
    health = maxHealth;
    sp = 0;
    RefillStamina();
    isDead = false;
    resultPoseAnimLocked = false;
    isHit = false;
    hitStunTimer = 0;
    currentState = YOSUKE_INTRO;
    currentFrame = 0;
    animAccumulator = 0;
    maxFrame = GetMaxFrameForState(YOSUKE_INTRO);
    jumpCount = 0;
    jumpHorizontalSpeed = 0.0f;
    verticalVelocity = 0.0f;
    hitThisAttack = false;
    dashHasHit = false;
    attackButtonHeld = false;
    crescentButtonHeld = false;
    skillHit = false;
    jumpSpaceWasReleased = true;
    personaAnimAccumulator = 0;
    jiraiyaFrame = 0;
    effectFrame = 0;
    showJiraiya = false;
    showEffect = false;
    noInputFrames = 0;
    idleWaitFrames = 0;
    introDisplayHold = 0;
    introLastFrame = 0;
    spaceChordBuffer = 0;
    spaceWasDown = false;
    damageGroundHold = 0;
    UpdateScaledHurtbox();
}

bool LoadYosukeTextures() {
    struct TextureLoadInfo {
        YosukeTexture* tex;
        const char* path;
        int frameCount;
    };

    TextureLoadInfo textures[] = {
        { &g_Stance, "assets/yosuke/stance.png", 4 },
        { &g_Walk, "assets/yosuke/walk.png", 4 },
        { &g_Run, "assets/yosuke/run.png", 4 },
        { &g_Dash, "assets/yosuke/dash.png", 2 },
        { &g_BackDash, "assets/yosuke/back_dash.png", 2 },
        { &g_Jump, "assets/yosuke/jump.png", 6 },
        { &g_Guard, "assets/yosuke/guard.png", 4 },
        { &g_Attack, "assets/yosuke/attack_combo.png", 30 },
        { &g_AirCombo, "assets/yosuke/air_combo.png", 13 },
        { &g_CrescentSlash, "assets/yosuke/crescent_slash.png", 1 },
        { &g_Moonsault, "assets/yosuke/moonsault.png", 4 },
        { &g_FlyingKunai, "assets/yosuke/flying_kunai.png", 6 },
        { &g_MirageSlash, "assets/yosuke/mirage_slash.png", 6 },
        { &g_BraveBlade, "assets/yosuke/brave_blade.png", 8 },
        { &g_PersonaAttack, "assets/yosuke/persona_attack.png", 6 },
        { &g_Garudyne, "assets/yosuke/garudyne.png", 8 },
        { &g_Damage, "assets/yosuke/damage.png", 7 },
        { &g_Recover, "assets/yosuke/recover.png", 5 },
        { &g_Intro, "assets/yosuke/intro.png", 8 },
        { &g_Win, "assets/yosuke/win.png", 9 },
        { &g_JiraiyaAttack, "assets/yosuke/jiraiya_attack.png", 23 },
        { &g_JiraiyaMirageSlash, "assets/yosuke/jiraiya_mirage_slash.png", 5 },
        { &g_JiraiyaBraveBlade, "assets/yosuke/jiraiya_brave_blade.png", 9 },
        { &g_JiraiyaGarudyne, "assets/yosuke/jiraiya_garudyne.png", 3 },
        { &g_JiraiyaWin, "assets/yosuke/jiraiya_win.png", 3 },
        { &g_FlyingKunaiEffect, "assets/yosuke/flying_kunai_effect.png", 3 },
        { &g_JiraiyaAttackEffect, "assets/yosuke/jiraiya_attack_effect.png", 4 },
    };

    for (int i = 0; i < (int)(sizeof(textures) / sizeof(textures[0])); ++i) {
        if (!LoadSheet(*textures[i].tex, textures[i].path, textures[i].frameCount)) {
            char msg[512];
            sprintf_s(msg, "Failed to load %s", textures[i].path);
            MessageBox(g_hWnd, msg, "Yosuke Texture Error", MB_OK);
            return false;
        }
    }

    if (!LoadSheet(g_GuardAir, "assets/yosuke/guard_air.png", 4)) {
        g_GuardAir = g_Guard;
    }

    return true;
}

void CleanUpYosukeTextures() {
    ReleaseSheet(g_Stance);
    ReleaseSheet(g_Walk);
    ReleaseSheet(g_Run);
    ReleaseSheet(g_Dash);
    ReleaseSheet(g_BackDash);
    ReleaseSheet(g_Jump);
    ReleaseSheet(g_Guard);
    if (g_GuardAir.texture && g_GuardAir.texture != g_Guard.texture) {
        ReleaseSheet(g_GuardAir);
    }
    ReleaseSheet(g_Attack);
    ReleaseSheet(g_AirCombo);
    ReleaseSheet(g_CrescentSlash);
    ReleaseSheet(g_Moonsault);
    ReleaseSheet(g_FlyingKunai);
    ReleaseSheet(g_MirageSlash);
    ReleaseSheet(g_BraveBlade);
    ReleaseSheet(g_PersonaAttack);
    ReleaseSheet(g_Garudyne);
    ReleaseSheet(g_Damage);
    ReleaseSheet(g_Recover);
    ReleaseSheet(g_Intro);
    ReleaseSheet(g_Win);
    ReleaseSheet(g_JiraiyaAttack);
    ReleaseSheet(g_JiraiyaMirageSlash);
    ReleaseSheet(g_JiraiyaBraveBlade);
    ReleaseSheet(g_JiraiyaGarudyne);
    ReleaseSheet(g_JiraiyaWin);
    ReleaseSheet(g_FlyingKunaiEffect);
    ReleaseSheet(g_JiraiyaAttackEffect);
}
