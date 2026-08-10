#include "Makoto.h"
#include "../../config.h"
#include "../../renderer.h"
#include "../joker/Joker.h"
#include "../../game_logic.h"
#include <cmath>
#include <float.h>
#include <stdio.h>

extern AttackData attackHitbox;
extern AttackData sideAttackHitbox;
extern AttackData attackUpHitbox;
extern AttackData downAttackHitbox;
extern Makoto g_Player1;
extern Joker g_Player2;
extern SpriteSheetBounds g_MessiahSheetBounds;
extern SpriteSheetBounds g_MegidolaonBurstBounds;
extern SpriteSheetBounds g_MegidolaonBlastBounds;

struct MakotoTexture {
    LPDIRECT3DTEXTURE9 texture;
    int cols;
    int rows;
    int maxFrame;
};

static MakotoTexture makotoStance;
static MakotoTexture makotoIdle;
static MakotoTexture makotoWalk;
static MakotoTexture makotoRun;
static MakotoTexture makotoDash;
static MakotoTexture makotoJump;
static MakotoTexture makotoAttack;
static MakotoTexture makotoCrouch;
static MakotoTexture makotoCrouchAtk;
static MakotoTexture makotoDodge;
static MakotoTexture makotoGuard;
static MakotoTexture makotoGuardAir;
static MakotoTexture makotoSideAttack;
static MakotoTexture makotoAttackUp;
static MakotoTexture makotoDownAttack;
static MakotoTexture makotoNeutralAir;
static MakotoTexture makotoUpAir;
static MakotoTexture makotoSideAir;
static MakotoTexture makotoDownAir;
static MakotoTexture makotoIntro;
static MakotoTexture makotoTaunt;
static MakotoTexture makotoDamage;
static MakotoTexture makotoRecover;
static MakotoTexture makotoSummon1;
static MakotoTexture makotoSummon2;
static MakotoTexture makotoSummonAir;
static MakotoTexture makotoSummonAir2;
static MakotoTexture makotoOrpheus;
static MakotoTexture makotoJackFrost;
static MakotoTexture makotoAGI;
static MakotoTexture makotoMabufu;
static MakotoTexture makotoMaziodyne;
static MakotoTexture makotoThanatosMaziodyne;
static MakotoTexture makotoThanatosSlash;
static MakotoTexture makotoMessiah;
static MakotoTexture makotoMegidolaon;
static MakotoTexture makotoWinTex;
static MakotoTexture makotoThanatosWin;

static const int makotoSpriteWidth = MAKOTO_CELL_SIZE;
static const int makotoSpriteHeight = MAKOTO_CELL_SIZE;
static RECT makotoRect;

static const MakotoTexture* GetTextureForState(int state) {
    switch (state) {
    case STANCE: return &makotoStance;
    case IDLE: return &makotoIdle;
    case WALK: return &makotoWalk;
    case RUN: return &makotoRun;
    case DASH: return &makotoDash;
    case JUMP: return &makotoJump;
    case ATTACK: return &makotoAttack;
    case CROUCH: return &makotoCrouch;
    case CROUCH_ATTACK: return &makotoCrouchAtk;
    case DODGE_FORWARD: case DODGE_BACKWARD: return &makotoDodge;
    case GUARD: return &makotoGuard;
    case GUARD_AIR: return &makotoGuardAir;
    case SIDE_ATTACK: return &makotoSideAttack;
    case ATTACK_UP: return &makotoAttackUp;
    case DOWN_ATTACK: return &makotoDownAttack;
    case NEUTRAL_AIR: return &makotoNeutralAir;
    case UP_AIR: return &makotoUpAir;
    case SIDE_AIR: return &makotoSideAir;
    case DOWN_AIR: return &makotoDownAir;
    case INTRO: return &makotoIntro;
    case TAUNT: return &makotoTaunt;
    case DAMAGE: return &makotoDamage;
    case RECOVER: return &makotoRecover;
    case SUMMON_1: return &makotoSummon1;
    case SUMMON_2: return &makotoSummon2;
    case SUMMON_AIR: return &makotoSummonAir;
    case SUMMON_AIR_2: return &makotoSummonAir2;
    case SUMMON_AIR_THANATOS: return &makotoSummonAir;
    case MAKOTO_WIN: return &makotoWinTex;
    case THANATOS_WIN: return &makotoThanatosWin;
    case SUMMON_1_ORPHEUS: case SUMMON_2_JACKFROST:
    case SUMMON_AIR_MAZIODYNE: case SUMMON_AIR_MESSIAH: case THANATOS_SLASH:
        return &makotoStance;
    default: return NULL;
    }
}

static int GetMaxFrameForState(int state) {
    const MakotoTexture* tex = GetTextureForState(state);
    return tex ? tex->maxFrame : 1;
}

static int ClampMakotoFrameIndex(const MakotoTexture& tex, int frameIndex) {
    if (tex.maxFrame <= 0) return 0;
    if (frameIndex < 0) return 0;
    if (frameIndex >= tex.maxFrame) return tex.maxFrame - 1;
    return frameIndex;
}

// Build a source RECT from a grid sprite sheet:
// left/top = cellSize * (frame % cols, frame / cols). Never hardcode each frame RECT.
static void SetMakotoFrameRect(RECT& rect, const MakotoTexture& tex, int frameIndex) {
    int frame = ClampMakotoFrameIndex(tex, frameIndex);
    rect.left = makotoSpriteWidth * (frame % tex.cols);
    rect.top = makotoSpriteHeight * (frame / tex.cols);
    rect.right = rect.left + makotoSpriteWidth;
    rect.bottom = rect.top + makotoSpriteHeight;
}

// Idle feet Y measured from each idle-cell sprite (pixel row of soles inside the 256 cell).
static float GetMakotoIdleFeetY(int frameIndex) {
    static const float kIdleFeetY[] = {
        54.0f, 55.0f, 55.0f, 64.0f, 69.0f, 69.0f, 69.0f, 69.0f, 55.0f, 54.0f
    };
    if (frameIndex < 0) frameIndex = 0;
    if (frameIndex >= (int)(sizeof(kIdleFeetY) / sizeof(kIdleFeetY[0]))) {
        frameIndex = (int)(sizeof(kIdleFeetY) / sizeof(kIdleFeetY[0])) - 1;
    }
    return kIdleFeetY[frameIndex];
}

static void GetMakotoBodyDrawAnchor(int state, int frameIndex, float& bodyHeight, float& feetY) {
    bodyHeight = MAKOTO_BODY_HEIGHT;
    feetY = MAKOTO_FEET_Y;
    if (state == IDLE) {
        feetY = GetMakotoIdleFeetY(frameIndex);
    }
}

static void DrawMakotoEffectSprite(
    LPD3DXSPRITE sprite,
    const MakotoTexture& tex,
    int frameIndex,
    const D3DXVECTOR3& pos,
    D3DCOLOR color,
    int facingDirection,
    float scale)
{
    if (!tex.texture) return;
    SetMakotoFrameRect(makotoRect, tex, frameIndex);
    DrawScaledCharacterSprite(sprite, tex.texture, &makotoRect, pos, facingDirection, scale, color, (float)MAKOTO_CELL_SIZE, MAKOTO_FEET_Y);
}

static void DrawMakotoCenteredEffectSprite(
    LPD3DXSPRITE sprite,
    const MakotoTexture& tex,
    int frameIndex,
    const D3DXVECTOR3& centerPos,
    float scale,
    D3DCOLOR color)
{
    if (!tex.texture) return;
    SetMakotoFrameRect(makotoRect, tex, frameIndex);
    DrawCenteredEffectSprite(sprite, tex.texture, &makotoRect, centerPos, scale, color, (float)MAKOTO_CELL_SIZE);
}

static bool AllowsMakotoMovement(int state, bool superActive) {
    if (superActive) return false;
    switch (state) {
    case STANCE:
    case WALK:
    case RUN:
    case CROUCH:
    case GUARD:
    case GUARD_AIR:
        return true;
    default:
        return false;
    }
}

static bool AdvanceOneShotFrame(int& accumulator, int& frame, int steps, int ticksPerFrame, int maxFrame) {
    if (maxFrame <= 0) return false;
    accumulator += steps;
    while (accumulator >= ticksPerFrame && frame < maxFrame - 1) {
        accumulator -= ticksPerFrame;
        frame++;
    }
    return frame >= maxFrame - 1;
}

static void AdvanceLoopFrame(int& accumulator, int& frame, int steps, int ticksPerFrame, int frameCount) {
    if (frameCount <= 0) return;
    accumulator += steps;
    while (accumulator >= ticksPerFrame) {
        accumulator -= ticksPerFrame;
        frame = (frame + 1) % frameCount;
    }
}

static bool IsGameKeyDown(int dik) {
    return g_WindowHasFocus && (diKeys[dik] & 0x80);
}

static bool IsGameMouseDown(int vk) {
    return g_WindowHasFocus && (GetAsyncKeyState(vk) & 0x8000);
}

static bool ResetsAnimationOnEnter(int state) {
    switch (state) {
    case STANCE:
    case WALK:
    case RUN:
    case CROUCH:
    case GUARD:
    case GUARD_AIR:
        return false;
    default:
        return true;
    }
}

static void BuildMakotoAttackBox(
    const D3DXVECTOR3& makotoPos,
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
    float forward = data.offsetX * scale;
    float vertical = data.offsetY * scale;

    if (facingDirection == 1) {
        attackX = makotoPos.x + forward;
    }
    else {
        attackX = makotoPos.x - forward - boxW;
    }
    attackY = makotoPos.y + vertical;
}

static AABB BuildCenteredHitbox(const D3DXVECTOR3& center, float halfW, float halfH) {
    return { center.x - halfW, center.y - halfH, halfW * 2.0f, halfH * 2.0f };
}

static void TrySkillHit(Joker& enemy, bool& hitFlag, int damage, const AABB& effectBox) {
    if (hitFlag || enemy.IsDead()) return;
    if (!CollisionHelper::AABBIntersect(effectBox, enemy.GetHurtbox())) return;
    enemy.ApplySkillDamage(damage);
    hitFlag = true;
}

static void TrySkillHitOnTarget(Joker& enemy, bool& hitFlag, int damage) {
    if (hitFlag || enemy.IsDead()) return;
    enemy.ApplySkillDamage(damage);
    hitFlag = true;
}

static D3DXVECTOR3 GetEnemyHurtboxCenter(Joker& enemy) {
    const AABB& hb = enemy.GetHurtbox();
    return D3DXVECTOR3(hb.x + hb.width * 0.5f, hb.y + hb.height * 0.5f, 0.0f);
}

static D3DXVECTOR3 GetAgiMabufuPos(Joker& enemy) {
    const AABB& hb = enemy.GetHurtbox();
    float centerX = hb.x + hb.width * 0.5f;
    float centerY = hb.y + hb.height * 0.5f;
    return D3DXVECTOR3(centerX, centerY, 0.0f);
}

static AABB GetMakotoBodyCollisionBox(const Makoto& makoto) {
    float s = GetMakotoDrawScale();
    AABB box;
    box.width = MAKOTO_BODY_WIDTH * s;
    box.height = MAKOTO_BODY_HEIGHT * s;
    box.x = makoto.position.x - box.width * 0.5f;
    box.y = makoto.position.y - box.height;
    return box;
}

static void ResolveFighterBodyOverlap(Makoto& makoto, Joker& joker) {
    const AABB makotoBox = GetMakotoBodyCollisionBox(makoto);
    const AABB jokerBox = joker.GetBodyCollisionBox();
    if (!CollisionHelper::AABBIntersect(makotoBox, jokerBox)) return;

    const float overlapLeft = (makotoBox.x + makotoBox.width) - jokerBox.x;
    const float overlapRight = (jokerBox.x + jokerBox.width) - makotoBox.x;
    if (overlapLeft <= 0.0f && overlapRight <= 0.0f) return;

    float separation = 0.0f;
    float makotoDelta = 0.0f;
    float jokerDelta = 0.0f;

    if (overlapLeft > 0.0f && overlapLeft <= overlapRight) {
        separation = overlapLeft;
        makotoDelta = -separation;
        jokerDelta = separation;
    }
    else if (overlapRight > 0.0f) {
        separation = overlapRight;
        makotoDelta = separation;
        jokerDelta = -separation;
    }

    if (JOKER_SANDBAG_MODE) {
        makoto.position.x += makotoDelta;
        ClampMakotoCenterX(makoto.position.x);
    }
    else {
        makoto.position.x += makotoDelta * 0.5f;
        joker.position.x += jokerDelta * 0.5f;
        ClampMakotoCenterX(makoto.position.x);
        ClampJokerCenterX(joker.position.x);
    }

    makoto.UpdateScaledHurtbox();
}

static D3DXVECTOR3 GetThanatosMaziodynePos(const D3DXVECTOR3& makotoPos, int facingDirection) {
    float s = GetPersonaEffectDrawScale();
    return D3DXVECTOR3(
        makotoPos.x + (float)facingDirection * THANATOS_MAZIODYNE_FORWARD_OFFSET * s,
        makotoPos.y - THANATOS_MAZIODYNE_VERTICAL_OFFSET * s,
        0.0f);
}

static int GetSkillFacingDirection(const D3DXVECTOR3& makotoPos, const D3DXVECTOR3& enemyCenter) {
    return (enemyCenter.x >= makotoPos.x) ? 1 : -1;
}

static D3DXVECTOR3 GetOrpheusPosBehindMakoto(const D3DXVECTOR3& makotoPos, int facingDirection) {
    return D3DXVECTOR3(
        makotoPos.x - (float)facingDirection * ORPHEUS_BEHIND_HORIZONTAL,
        makotoPos.y - ORPHEUS_BEHIND_VERTICAL,
        0.0f);
}

static D3DXVECTOR3 GetEffectPosFromEnemyCenter(
    const D3DXVECTOR3& enemyCenter, int facingDirection,
    float horizontalOffsetTowardMakoto, float verticalOffsetUp) {
    return D3DXVECTOR3(
        enemyCenter.x - (float)facingDirection * horizontalOffsetTowardMakoto,
        enemyCenter.y - verticalOffsetUp,
        0.0f);
}

static D3DXVECTOR3 GetMaziodynePos(const D3DXVECTOR3& enemyCenter, int facingDirection) {
    return GetEffectPosFromEnemyCenter(
        enemyCenter, facingDirection,
        MAZIODYNE_HORIZONTAL_OFFSET, MAZIODYNE_VERTICAL_OFFSET);
}

static D3DXVECTOR3 GetMaziodyneThanatosPos(const D3DXVECTOR3& makotoPos, int facingDirection) {
    return GetThanatosMaziodynePos(makotoPos, facingDirection);
}

static D3DXVECTOR3 GetMessiahPos(const D3DXVECTOR3& enemyCenter, int facingDirection) {
    return GetEffectPosFromEnemyCenter(
        enemyCenter, facingDirection,
        MESSIAH_HORIZONTAL_OFFSET, MESSIAH_VERTICAL_OFFSET);
}

static D3DXVECTOR3 GetMegidolaonPos(const D3DXVECTOR3& enemyCenter, int facingDirection) {
    return GetEffectPosFromEnemyCenter(
        enemyCenter, facingDirection,
        MEGIDOLAON_HORIZONTAL_OFFSET, MEGIDOLAON_VERTICAL_OFFSET);
}

static D3DXVECTOR3 GetThanatosSlashPos(const D3DXVECTOR3& enemyCenter, int facingDirection) {
    return GetEffectPosFromEnemyCenter(
        enemyCenter, facingDirection,
        THANATOS_SLASH_HORIZONTAL_OFFSET, THANATOS_SLASH_VERTICAL_OFFSET);
}

Makoto::Makoto()
    : currentFrame(0), maxFrame(1), frameCounter(0), currentState(INTRO)
    , jumpCount(0), jumpHorizontalSpeed(0), verticalVelocity(0)
    , isSuperMoveActive(false), isMakotoGray(false), superMoveTimer(0), overlayColor(0)
    , isOrpheusActive(false), isJackFrostActive(false), isAGIActive(false), isMabufuActive(false)
    , isThanatosActive(false), isMaziodyneActive(false), isThanatosSlashActive(false)
    , isMessiahActive(false), isMegidolaonActive(false)
    , orpheusAnimationComplete(false), jackfrostAnimationComplete(false)
    , agiAnimationComplete(false), mabufuAnimationComplete(false)
    , thanatosAnimationComplete(false), maziodyneAnimationComplete(false)
    , thanatosSlashAnimationComplete(false)
    , messiahAnimationComplete(false), megidolaonAnimationComplete(false)
    , orpheusFrame(0), jackfrostFrame(0), agiFrame(0), mabufuFrame(0)
    , thanatosFrame(0), maziodyneFrame(0), slashFrame(0), messiahFrame(0), megidolaonFrame(0)
    , hitAGI(false), hitMabufu(false), hitMaziodyne(false), hitSlash(false), hitMegidolaon(false)
    , hitThisAttack(false), dashHasHit(false)
    , spaceChordBuffer(0), spaceWasDown(false), messiahChordConsumed(false)
    , slashAnimTimer(0), slashTotalFrames(0)
    , animAccumulator(0), personaAnimAccumulator(0), noInputFrames(0)
    , introDisplayHold(0), introLastFrame(0), stanceEntryDelay(0)
    , actionVisualHold(0), actionHoldState(STANCE), actionHoldFrame(0)
    , meleeHitSparkActive(false), meleeHitSparkFrame(0), meleeHitSparkPos(0, 0, 0)
{
    float spawnX = GetMakotoScreenHalfWidth() + MAKOTO_WINDOW_MARGIN + MAKOTO_SPAWN_FORWARD;
    position = D3DXVECTOR3(spawnX, CHARACTER_GROUND_Y, 0);
    facingDirection = 1;
    health = MAKOTO_MAX_HEALTH;
    maxHealth = MAKOTO_MAX_HEALTH;
    velocity = MAKOTO_MOVE_SPEED;
    maxFrame = GetMaxFrameForState(INTRO);
    UpdateScaledHurtbox();
}

Makoto::~Makoto() {}

bool Makoto::IsOnGround() const {
    return position.y >= CHARACTER_GROUND_Y - 0.5f;
}

void Makoto::ApplyGravity(int steps) {
    for (int step = 0; step < steps; ++step) {
        if (IsOnGround() && verticalVelocity >= 0.0f) {
            position.y = CHARACTER_GROUND_Y;
            verticalVelocity = 0.0f;
            continue;
        }
        verticalVelocity += GRAVITY;
        position.y += verticalVelocity;
        if (position.y >= CHARACTER_GROUND_Y) {
            position.y = CHARACTER_GROUND_Y;
            verticalVelocity = 0.0f;
        }
    }
}
bool Makoto::CanUseSpaceChord() const {
    return spaceChordBuffer > 0 ||
        currentState == JUMP ||
        currentState == NEUTRAL_AIR ||
        currentState == UP_AIR ||
        currentState == SIDE_AIR ||
        currentState == DOWN_AIR;
}

void Makoto::TickSpaceChordBuffer(bool isJumpPressed, int steps) {
    bool spaceJustPressed = isJumpPressed && !spaceWasDown;
    if (spaceJustPressed) {
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
void Makoto::BeginAirAttackState(int state, int frames) {
    currentState = state;
    currentFrame = 0;
    maxFrame = frames;
    animAccumulator = 0;
    hitThisAttack = false;
    if (position.y >= CHARACTER_GROUND_Y - 2.0f) {
        position.y = CHARACTER_GROUND_Y - 50.0f * GetCharacterRenderScale() * 0.35f;
    }
    verticalVelocity = 0.0f;
    jumpCount = 1;
}

void Makoto::OnMeleeHitConnected(Joker& enemy) {
    RestoreSp(SP_GAIN_ON_HIT);
    const AABB& hb = enemy.GetHurtbox();
    meleeHitSparkPos = D3DXVECTOR3(hb.x + hb.width * 0.5f, hb.y + hb.height * 0.35f, 0.0f);
    meleeHitSparkActive = true;
    meleeHitSparkFrame = 0;
}

void Makoto::UpdateMeleeHitSpark(int steps) {
    if (!meleeHitSparkActive) return;
    meleeHitSparkFrame += steps;
    if (meleeHitSparkFrame >= 10) {
        meleeHitSparkActive = false;
    }
}

void Makoto::RenderMeleeHitSpark(LPD3DXSPRITE sprite) {
    if (!sprite || !meleeHitSparkActive) return;

    const float t = (float)meleeHitSparkFrame / 10.0f;
    const float pulse = 1.0f - t;
    const int alpha = (int)(255.0f * pulse);
    if (alpha <= 0) return;

    const float coreSize = 10.0f + t * 28.0f;
    const float ringSize = coreSize * 1.45f;
    const float sparkArm = coreSize * 0.65f;
    const float sparkThickness = 4.0f + t * 2.0f;
    const float cx = meleeHitSparkPos.x;
    const float cy = meleeHitSparkPos.y;

    DrawDebugRect(sprite, cx - coreSize * 0.5f, cy - coreSize * 0.5f, coreSize, coreSize,
        D3DCOLOR_ARGB(alpha, 255, 255, 255));
    DrawDebugRect(sprite, cx - ringSize * 0.5f, cy - ringSize * 0.5f, ringSize, ringSize,
        D3DCOLOR_ARGB(alpha / 2, 210, 170, 255));
    DrawDebugRect(sprite, cx - sparkArm * 0.5f, cy - sparkThickness * 0.5f, sparkArm, sparkThickness,
        D3DCOLOR_ARGB(alpha, 255, 245, 170));
    DrawDebugRect(sprite, cx - sparkThickness * 0.5f, cy - sparkArm * 0.5f, sparkThickness, sparkArm,
        D3DCOLOR_ARGB(alpha, 255, 245, 170));
}

void Makoto::CheckAttackCollision(Joker& enemy) {
    if (enemy.IsDead()) return;

    AttackData* data = nullptr;
    int totalFrames = GetMaxFrameForState(currentState);
    float s = GetCharacterRenderScale();

    switch (currentState) {
    case ATTACK:        data = &attackHitbox;       break;
    case SIDE_ATTACK:   data = &sideAttackHitbox;   break;
    case ATTACK_UP:     data = &attackUpHitbox;     break;
    case DOWN_ATTACK:   data = &downAttackHitbox;   break;
    case CROUCH_ATTACK: data = &attackHitbox;       break;
    case NEUTRAL_AIR:   data = &attackHitbox;       break;
    case UP_AIR:        data = &attackUpHitbox;     break;
    case SIDE_AIR:      data = &sideAttackHitbox;   break;
    case DOWN_AIR:      data = &downAttackHitbox;   break;
    case DASH:          break;
    default: return;
    }
    if (currentState == DASH) {
        if (!dashHasHit &&
            currentFrame >= DASH_HIT_START_FRAME &&
            currentFrame <= DASH_HIT_END_FRAME) {
            float dashW = DASH_HITBOX_WIDTH * s;
            float dashH = DASH_HITBOX_HEIGHT * s;
            float dashX = 0.0f;
            if (facingDirection == 1) dashX = position.x + DASH_HITBOX_FORWARD * s;
            else dashX = position.x - DASH_HITBOX_FORWARD * s - dashW;
            float dashY = position.y - DASH_HITBOX_UP * s;

            AABB dashBox = { dashX, dashY, dashW, dashH };
            if (CollisionHelper::AABBIntersect(dashBox, enemy.GetHurtbox())) {
                const_cast<Joker&>(enemy).TakeDamage(DASH_HIT_DAMAGE);
                dashHasHit = true;
                OnMeleeHitConnected(enemy);
            }
        }
        return;
    }

    if (currentFrame == 0) hitThisAttack = false;

    int startF = max(1, totalFrames / 4);
    int endF = min(totalFrames - 1, data->endFrame);
    if (currentState == SIDE_AIR) {
        startF = 1;
        endF = totalFrames - 1;
    }
    if (currentState == DOWN_ATTACK || currentState == DOWN_AIR) {
        startF = 1;
        endF = min(totalFrames - 1, data->endFrame);
    }

    if (currentFrame >= startF && currentFrame <= endF) {
        float attackX = 0.0f;
        float attackY = 0.0f;
        float boxW = 0.0f;
        float boxH = 0.0f;
        BuildMakotoAttackBox(position, facingDirection, s, *data, attackX, attackY, boxW, boxH);

        AABB attackBox = { attackX, attackY, boxW, boxH };
        AABB enemyBox = enemy.GetHurtbox();

        if (!hitThisAttack && CollisionHelper::AABBIntersect(attackBox, enemyBox)) {
            const_cast<Joker&>(enemy).TakeDamage(data->damage);
            hitThisAttack = true;
            OnMeleeHitConnected(enemy);
        }
    }
}

void Makoto::CompleteOneShotToStance(int lastFrame) {
    (void)lastFrame;
    int stanceFrames = GetMaxFrameForState(STANCE);
    currentState = STANCE;
    currentFrame = 0;
    animAccumulator = 0;
    hitThisAttack = false;
    maxFrame = stanceFrames;
    actionVisualHold = 0;
    actionHoldState = STANCE;
    actionHoldFrame = 0;
}

void Makoto::TickVisualHolds(int animSteps) {
    if (actionVisualHold > 0) {
        actionVisualHold -= animSteps;
        if (actionVisualHold < 0) actionVisualHold = 0;
    }
    if (stanceEntryDelay > 0) {
        stanceEntryDelay -= animSteps;
        if (stanceEntryDelay < 0) stanceEntryDelay = 0;
    }
}

void Makoto::BeginMaziodyneSuper() {
    currentState = SUMMON_AIR_MAZIODYNE;
    currentFrame = 0;
    animAccumulator = 0;
    personaAnimAccumulator = 0;
    verticalVelocity = 0.0f;
    isSuperMoveActive = true;
    superMoveTimer = 0;
    overlayColor = D3DCOLOR_ARGB(0, 0, 0, 0);
    thanatosPos = GetThanatosMaziodynePos(position, facingDirection);
    thanatosFrame = 0;
    isThanatosActive = true;
    thanatosAnimationComplete = false;
    maziodyneFrame = 0;
    isMaziodyneActive = true;
    maziodyneAnimationComplete = false;
    hitMaziodyne = false;
}

void Makoto::BeginMessiahSuper() {
    currentState = SUMMON_AIR_MESSIAH;
    currentFrame = 0;
    animAccumulator = 0;
    personaAnimAccumulator = 0;
    maxFrame = GetMaxFrameForState(STANCE);
    verticalVelocity = 0.0f;
    spaceChordBuffer = 0;
    isSuperMoveActive = true;
    superMoveTimer = 0;
    overlayColor = D3DCOLOR_ARGB(0, 0, 0, 0);
    messiahFrame = 0;
    isMessiahActive = true;
    messiahAnimationComplete = false;
    megidolaonFrame = 0;
    isMegidolaonActive = true;
    megidolaonAnimationComplete = false;
    hitMegidolaon = false;
}

void Makoto::UpdateLiveEffectPositions(Joker& enemy) {
    const D3DXVECTOR3 enemyCenter = GetEnemyHurtboxCenter(enemy);
    const D3DXVECTOR3 agiMabufuPos = GetAgiMabufuPos(enemy);
    const int skillFacing = GetSkillFacingDirection(position, enemyCenter);
    currentEnemyPos = enemyCenter;

    if (isOrpheusActive) {
        orpheusPos = GetOrpheusPosBehindMakoto(position, facingDirection);
    }
    if (isAGIActive) {
        agiPos = agiMabufuPos;
    }
    if (isMabufuActive) {
        mabufuPos = agiMabufuPos;
    }
    if (isThanatosActive) {
        thanatosPos = GetThanatosMaziodynePos(position, facingDirection);
    }
    if (isMaziodyneActive) {
        maziodynePos = GetMaziodynePos(enemyCenter, skillFacing);
    }
    if (isThanatosSlashActive) {
        slashPos = GetThanatosSlashPos(enemyCenter, skillFacing);
    }
    if (isMessiahActive) {
        messiahPos = GetMessiahPos(enemyCenter, skillFacing);
    }
    if (isMegidolaonActive) {
        megidolaonPos = agiMabufuPos;
    }
}

void Makoto::Update() {
    if (isDead) return;

    struct ResolveOverlapOnExit {
        ~ResolveOverlapOnExit() { ResolveFighterBodyOverlap(g_Player1, g_Player2); }
    } resolveOverlapOnExit;

    int steps = g_GameTimer.FramesToUpdate();
    if (steps <= 0) return;
    if (steps > 4) steps = 4;
    const int animSteps = 1;

    UpdateMeleeHitSpark(steps);
    TickVisualHolds(animSteps);

    if (isAGIActive || isMabufuActive || isMaziodyneActive || isMegidolaonActive ||
        isMessiahActive || isThanatosActive || isThanatosSlashActive || isOrpheusActive) {
        UpdateLiveEffectPositions(g_Player2);
    }

    currentEnemyPos = g_Player2.GetPosition();

    if (isSuperMoveActive ||
        currentState == SUMMON_1_ORPHEUS ||
        currentState == SUMMON_2_JACKFROST ||
        currentState == SUMMON_AIR_MAZIODYNE ||
        currentState == THANATOS_SLASH ||
        currentState == SUMMON_AIR_MESSIAH) {
        UpdatePersonaLogic(g_Player2, steps);
        return;
    }

    bool isRunning = IsGameKeyDown(DIK_LSHIFT) || IsGameKeyDown(DIK_RSHIFT);
    int currentVelocity = isRunning ? (velocity * 2) : velocity;
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

    if (AllowsMakotoMovement(currentState, isSuperMoveActive) &&
        isMoving && currentState != DASH && currentState != DODGE_FORWARD && currentState != DODGE_BACKWARD) {
        position.x += moveDirX * currentVelocity * steps;
        ClampMakotoCenterX(position.x);
    }

    if (currentState == INTRO) {
        maxFrame = GetMaxFrameForState(INTRO);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, MAKOTO_INTRO_TICKS, maxFrame)) {
            introLastFrame = maxFrame - 1;
            int stanceFrames = GetMaxFrameForState(STANCE);
            currentState = STANCE;
            currentFrame = 0;
            animAccumulator = 0;
            noInputFrames = 0;
            maxFrame = stanceFrames;
        }
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == TAUNT) {
        maxFrame = GetMaxFrameForState(TAUNT);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, MAKOTO_IDLE_TICKS, maxFrame)) {
            CompleteOneShotToStance(maxFrame - 1);
        }
        UpdateScaledHurtbox();
        return;
    }
    if (currentState == IDLE) {
        bool checkInput = isMoving ||
            IsGameKeyDown(DIK_SPACE) ||
            IsGameKeyDown(DIK_J) ||
            IsGameMouseDown(VK_LBUTTON) ||
            IsGameMouseDown(VK_RBUTTON) ||
            IsGameKeyDown(DIK_I) ||
            IsGameKeyDown(DIK_C) ||
            IsGameKeyDown(DIK_E) ||
            IsGameKeyDown(DIK_R) ||
            IsGameKeyDown(DIK_S) ||
            IsGameKeyDown(DIK_T) ||
            IsGameKeyDown(DIK_1) ||
            IsGameKeyDown(DIK_2) ||
            IsGameKeyDown(DIK_3) ||
            IsGameKeyDown(DIK_4) ||
            IsGameKeyDown(DIK_5);
        if (checkInput) {
            currentState = STANCE;
            currentFrame = 0;
            animAccumulator = 0;
            maxFrame = GetMaxFrameForState(STANCE);
            UpdateScaledHurtbox();
            return;
        }
        maxFrame = GetMaxFrameForState(IDLE);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, MAKOTO_IDLE_PLAY_TICKS, maxFrame)) {
            noInputFrames = 0;
            CompleteOneShotToStance(maxFrame - 1);
        }
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == DODGE_FORWARD) {
        position.x += (velocity * DODGE_SLIDE_SPEED) * steps;
        ClampMakotoCenterX(position.x);
        maxFrame = GetMaxFrameForState(DODGE_FORWARD);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, MAKOTO_ACTION_TICKS, maxFrame)) {
            CompleteOneShotToStance(maxFrame - 1);
        }
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == DODGE_BACKWARD) {
        position.x -= (velocity * DODGE_SLIDE_SPEED) * steps;
        ClampMakotoCenterX(position.x);
        maxFrame = GetMaxFrameForState(DODGE_BACKWARD);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, MAKOTO_ACTION_TICKS, maxFrame)) {
            CompleteOneShotToStance(maxFrame - 1);
        }
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == DASH) {
        position.x += (float)facingDirection * (velocity * MAKOTO_DASH_SPEED_MULTIPLIER) * steps;
        ClampMakotoCenterX(position.x);
        maxFrame = GetMaxFrameForState(DASH);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, MAKOTO_ACTION_TICKS, maxFrame)) {
            dashHasHit = false;
            CompleteOneShotToStance(maxFrame - 1);
        }
        CheckAttackCollision(g_Player2);
        UpdateScaledHurtbox();
        return;
    }
    if (currentState == JUMP) {
        bool isLeftPressed = IsGameKeyDown(DIK_LEFT) || IsGameKeyDown(DIK_A);
        bool isRightPressed = IsGameKeyDown(DIK_RIGHT) || IsGameKeyDown(DIK_D);
        bool isJumpPressed = IsGameKeyDown(DIK_SPACE);
        bool isEPressed = IsGameKeyDown(DIK_E);
        bool isRPressed = IsGameKeyDown(DIK_R);
        bool isAttackPressed = IsGameMouseDown(VK_LBUTTON);
        bool isKey3 = IsGameKeyDown(DIK_3);
        bool isKey4 = IsGameKeyDown(DIK_4);

        TickSpaceChordBuffer(isJumpPressed, steps);

        if (isLeftPressed) facingDirection = -1;
        else if (isRightPressed) facingDirection = 1;

        if (isKey3 && (CanUseSpaceChord() || isJumpPressed) && TryConsumeSp(SP_COST_SUMMON_AIR)) {
            currentState = SUMMON_AIR;
            currentFrame = 0;
            maxFrame = GetMaxFrameForState(SUMMON_AIR);
            animAccumulator = 0;
            verticalVelocity = 0.0f;
            isThanatosActive = false;
            isMaziodyneActive = false;
            thanatosFrame = 0;
            maziodyneFrame = 0;
            thanatosAnimationComplete = false;
            maziodyneAnimationComplete = false;
            isSuperMoveActive = false;
            hitMaziodyne = false;
            spaceChordBuffer = 0;
            UpdateScaledHurtbox();
            return;
        }
        if (isKey4 && (CanUseSpaceChord() || isJumpPressed) && !messiahChordConsumed && TryConsumeSp(SP_COST_SUMMON_AIR_2)) {
            currentState = SUMMON_AIR_2;
            currentFrame = 0;
            messiahChordConsumed = true;
            maxFrame = GetMaxFrameForState(SUMMON_AIR_2);
            animAccumulator = 0;
            verticalVelocity = 0.0f;
            isSuperMoveActive = false;
            superMoveTimer = 0;
            overlayColor = D3DCOLOR_ARGB(0, 0, 0, 0);
            messiahFrame = 0;
            isMessiahActive = false;
            messiahAnimationComplete = false;
            megidolaonFrame = 0;
            isMegidolaonActive = false;
            megidolaonAnimationComplete = false;
            hitMegidolaon = false;
            spaceChordBuffer = 0;
            UpdateScaledHurtbox();
            return;
        }
        if (isEPressed) {
            BeginAirAttackState(SIDE_AIR, GetMaxFrameForState(SIDE_AIR));
            UpdateScaledHurtbox();
            return;
        }
        if (isRPressed) {
            BeginAirAttackState(UP_AIR, GetMaxFrameForState(UP_AIR));
            UpdateScaledHurtbox();
            return;
        }
        if (isAttackPressed) {
            if (currentFrame >= 3) { BeginAirAttackState(DOWN_AIR, GetMaxFrameForState(DOWN_AIR)); }
            else { BeginAirAttackState(NEUTRAL_AIR, GetMaxFrameForState(NEUTRAL_AIR)); }
            UpdateScaledHurtbox();
            return;
        }        static bool spaceWasReleased = true;
        if (!isJumpPressed) spaceWasReleased = true;
        if (isJumpPressed && spaceWasReleased && jumpCount < 2) {
            jumpCount = 2;
            currentFrame = 0;
            spaceWasReleased = false;
        }
        position.x += jumpHorizontalSpeed * steps;
        ClampMakotoCenterX(position.x);

        float jumpOffsets[7] = { 0.0f, -16.0f, -24.0f, -32.0f, -14.0f, 14.0f, 48.0f };
        float js = GetCharacterRenderScale();
        maxFrame = GetMaxFrameForState(JUMP);
        int prevFrame = currentFrame;
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, MAKOTO_ACTION_TICKS, maxFrame)) {
            position.y = CHARACTER_GROUND_Y;
            jumpCount = 0;
            CompleteOneShotToStance(maxFrame - 1);
        }
        else {
            for (int f = prevFrame; f < currentFrame && f < 7; ++f) {
                position.y += jumpOffsets[f] * js * 0.55f;
            }
        }
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == SUMMON_1) {
        maxFrame = GetMaxFrameForState(SUMMON_1);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, MAKOTO_SUMMON_TICKS, maxFrame)) {
            currentFrame = maxFrame - 1;
            isSuperMoveActive = true; superMoveTimer = 0; overlayColor = D3DCOLOR_ARGB(0, 0, 0, 0);
            currentState = SUMMON_1_ORPHEUS; currentFrame = 0;
            animAccumulator = 0;
            orpheusPos = GetOrpheusPosBehindMakoto(position, facingDirection);
            orpheusFrame = 0; isOrpheusActive = true; orpheusAnimationComplete = false;
            isAGIActive = false; agiFrame = 0; agiAnimationComplete = false;
        }
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == SUMMON_2) {
        maxFrame = GetMaxFrameForState(SUMMON_2);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, MAKOTO_SUMMON_TICKS, maxFrame)) {
            currentFrame = maxFrame - 1;
            isSuperMoveActive = true; superMoveTimer = 0; overlayColor = D3DCOLOR_ARGB(0, 0, 0, 0);
            currentState = SUMMON_2_JACKFROST; currentFrame = 0;
            animAccumulator = 0;
            jackfrostPos = D3DXVECTOR3(position.x - (float)facingDirection * PERSONA_BEHIND_HORIZONTAL, position.y - JACKFROST_BEHIND_VERTICAL, 0);
            jackfrostFrame = 0; isJackFrostActive = true; jackfrostAnimationComplete = false;
            isMabufuActive = false; mabufuFrame = 0; mabufuAnimationComplete = false;
        }
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == SUMMON_AIR) {
        ApplyGravity(steps);
        ClampMakotoCenterX(position.x);
        maxFrame = GetMaxFrameForState(SUMMON_AIR);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, MAKOTO_SUMMON_TICKS, maxFrame)) {
            BeginMaziodyneSuper();
        }
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == SUMMON_AIR_2) {
        ApplyGravity(steps);
        ClampMakotoCenterX(position.x);
        maxFrame = GetMaxFrameForState(SUMMON_AIR_2);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, MAKOTO_SUMMON_TICKS, maxFrame)) {
            BeginMessiahSuper();
        }
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == NEUTRAL_AIR || currentState == UP_AIR || currentState == SIDE_AIR || currentState == DOWN_AIR) {
        maxFrame = GetMaxFrameForState(currentState);

        ApplyGravity(steps);

        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, MAKOTO_ACTION_TICKS, maxFrame)) {
            position.y = CHARACTER_GROUND_Y;
            verticalVelocity = 0.0f;
            jumpCount = 0;
            hitThisAttack = false;
            dashHasHit = false;
            CompleteOneShotToStance(maxFrame - 1);
        }
        CheckAttackCollision(g_Player2);
        UpdateScaledHurtbox();
        return;
    }

    if (currentState == ATTACK || currentState == CROUCH_ATTACK || currentState == SIDE_ATTACK ||
        currentState == ATTACK_UP || currentState == DOWN_ATTACK) {
        maxFrame = GetMaxFrameForState(currentState);
        if (AdvanceOneShotFrame(animAccumulator, currentFrame, animSteps, MAKOTO_ACTION_TICKS, maxFrame)) {
            CompleteOneShotToStance(maxFrame - 1);
        }
        CheckAttackCollision(g_Player2);
        UpdateScaledHurtbox();
        return;
    }

    bool isJumpPressed = IsGameKeyDown(DIK_SPACE);
    bool isDashPressed = IsGameKeyDown(DIK_J);
    bool isAttackPressed = IsGameMouseDown(VK_LBUTTON);
    bool isDodgePressed = IsGameMouseDown(VK_RBUTTON);
    bool isGuardPressed = IsGameKeyDown(DIK_I);
    bool isCrouchPressed = IsGameKeyDown(DIK_C);
    bool isSideAtkPressed = IsGameKeyDown(DIK_E);
    bool isAtkUpPressed = IsGameKeyDown(DIK_R);
    bool isDownPressed = IsGameKeyDown(DIK_S);
    bool isTauntPressed = IsGameKeyDown(DIK_T);

    bool isSummon1 = IsGameKeyDown(DIK_1);
    bool isSummon2 = IsGameKeyDown(DIK_2);
    bool isKey3 = IsGameKeyDown(DIK_3);
    bool isKey4 = IsGameKeyDown(DIK_4);
    if (!isKey4) {
        messiahChordConsumed = false;
    }
    TickSpaceChordBuffer(isJumpPressed, steps);
    bool canUseSpaceChord = CanUseSpaceChord() || isJumpPressed;
    bool isSummonAirThanatos = isKey3 && canUseSpaceChord;
    bool isSummonAirMessiah = isKey4 && canUseSpaceChord && !messiahChordConsumed;
    bool isSpaceAirSide = isSideAtkPressed && canUseSpaceChord;
    bool isSpaceAirUp = isAtkUpPressed && canUseSpaceChord;
    bool isThanatosSlashPressed = IsGameKeyDown(DIK_5);

    bool hasAnyInput = isMoving || isJumpPressed || isDashPressed || isAttackPressed || isDodgePressed ||
        isGuardPressed || isCrouchPressed || isSideAtkPressed || isAtkUpPressed || isDownPressed ||
        isTauntPressed || isSummon1 || isSummon2 || isSummonAirThanatos || isSummonAirMessiah ||
        isSpaceAirSide || isSpaceAirUp || isThanatosSlashPressed;

    const int IDLE_THRESHOLD_FRAMES = ::IDLE_THRESHOLD_FRAMES;
    if (hasAnyInput) noInputFrames = 0;
    else if (currentState == STANCE) noInputFrames += steps;
    int nextState = STANCE;

    if (currentState == NEUTRAL_AIR || currentState == UP_AIR || currentState == SIDE_AIR || currentState == DOWN_AIR) {
    }
    else if (isSummon1 && currentState != SUMMON_1 && currentState != SUMMON_1_ORPHEUS && TryConsumeSp(SP_COST_SUMMON_1)) {
        nextState = SUMMON_1; currentFrame = 0;
        isOrpheusActive = false; isAGIActive = false; orpheusFrame = 0; agiFrame = 0;
        orpheusAnimationComplete = false; agiAnimationComplete = false;
        isSuperMoveActive = false; hitAGI = false;
    }
    else if (isSummon2 && currentState != SUMMON_2 && currentState != SUMMON_2_JACKFROST && TryConsumeSp(SP_COST_SUMMON_2)) {
        nextState = SUMMON_2; currentFrame = 0;
        isJackFrostActive = false; isMabufuActive = false; jackfrostFrame = 0; mabufuFrame = 0;
        jackfrostAnimationComplete = false; mabufuAnimationComplete = false;
        isSuperMoveActive = false; hitMabufu = false;
    }
    else if (isSummonAirThanatos && currentState != SUMMON_AIR && currentState != SUMMON_AIR_MAZIODYNE && TryConsumeSp(SP_COST_SUMMON_AIR)) {
        nextState = SUMMON_AIR;
        currentFrame = 0;
        maxFrame = GetMaxFrameForState(SUMMON_AIR);
        animAccumulator = 0;
        isThanatosActive = false;
        isMaziodyneActive = false;
        thanatosFrame = 0;
        maziodyneFrame = 0;
        thanatosAnimationComplete = false;
        maziodyneAnimationComplete = false;
        isSuperMoveActive = false;
        hitMaziodyne = false;
        spaceChordBuffer = 0;
    }
    else if (isSummonAirMessiah && currentState != SUMMON_AIR_2 && currentState != SUMMON_AIR_MESSIAH && TryConsumeSp(SP_COST_SUMMON_AIR_2)) {
        nextState = SUMMON_AIR_2;
        currentFrame = 0;
        messiahChordConsumed = true;
        maxFrame = GetMaxFrameForState(SUMMON_AIR_2);
        animAccumulator = 0;
        isSuperMoveActive = false;
        superMoveTimer = 0;
        overlayColor = D3DCOLOR_ARGB(0, 0, 0, 0);
        messiahFrame = 0;
        isMessiahActive = false;
        messiahAnimationComplete = false;
        megidolaonFrame = 0;
        isMegidolaonActive = false;
        megidolaonAnimationComplete = false;
        hitMegidolaon = false;
        spaceChordBuffer = 0;
    }
    else if (isThanatosSlashPressed && TryConsumeSp(SP_COST_THANATOS_SLASH)) {
        slashAnimTimer = 0;
        slashTotalFrames = 0;
        nextState = THANATOS_SLASH; currentFrame = 0;
        isSuperMoveActive = true; superMoveTimer = 0; overlayColor = D3DCOLOR_ARGB(0, 0, 0, 0);
        isThanatosSlashActive = true; slashFrame = 0; hitSlash = false;
        thanatosSlashAnimationComplete = false;
        slashPos = GetThanatosSlashPos(GetEnemyHurtboxCenter(g_Player2), facingDirection);
    }
    else if (isTauntPressed) {
        nextState = TAUNT; currentFrame = 0;
    }
    else if (isSpaceAirSide) {
        nextState = SIDE_AIR; currentFrame = 0; maxFrame = GetMaxFrameForState(SIDE_AIR); animAccumulator = 0; hitThisAttack = false;        if (position.y >= CHARACTER_GROUND_Y - 2.0f) {
            position.y = CHARACTER_GROUND_Y - 50.0f * GetCharacterRenderScale() * 0.35f;
        }
        verticalVelocity = 0.0f; jumpCount = 1; spaceChordBuffer = 0;
    }
    else if (isSpaceAirUp) {
        nextState = UP_AIR; currentFrame = 0; maxFrame = GetMaxFrameForState(UP_AIR); animAccumulator = 0; hitThisAttack = false;        if (position.y >= CHARACTER_GROUND_Y - 2.0f) {
            position.y = CHARACTER_GROUND_Y - 50.0f * GetCharacterRenderScale() * 0.35f;
        }
        verticalVelocity = 0.0f; jumpCount = 1; spaceChordBuffer = 0;
    }
    else if (isJumpPressed) {
        nextState = JUMP; jumpCount = 1; currentFrame = 0;
        jumpHorizontalSpeed = (moveDirX != 0.0f) ? (moveDirX * (currentVelocity * 1.5f)) : 0.0f;
    }
    else if (isDashPressed) {
        nextState = DASH; dashHasHit = false;
    }
    else if (isDodgePressed) {
        bool isLeftPressed = IsGameKeyDown(DIK_LEFT) || IsGameKeyDown(DIK_A);
        bool isRightPressed = IsGameKeyDown(DIK_RIGHT) || IsGameKeyDown(DIK_D);
        if (isLeftPressed && !isRightPressed) nextState = DODGE_BACKWARD;
        else if (isRightPressed && !isLeftPressed) nextState = DODGE_FORWARD;
        else nextState = (facingDirection == 1) ? DODGE_BACKWARD : DODGE_FORWARD;
    }
    else if (isDownPressed && isAttackPressed) { nextState = DOWN_ATTACK; }
    else if (isSideAtkPressed) { nextState = SIDE_ATTACK; }
    else if (isAtkUpPressed) { nextState = ATTACK_UP; }
    else if (isCrouchPressed && isAttackPressed) { nextState = CROUCH_ATTACK; }
    else if (isAttackPressed) { nextState = ATTACK; }
    else if (isGuardPressed) { nextState = IsOnGround() ? GUARD : GUARD_AIR; }
    else if (isCrouchPressed && !isMoving) { nextState = CROUCH; }
    else if (isMoving) { nextState = isRunning ? RUN : WALK; }
    else {
        if (noInputFrames >= IDLE_THRESHOLD_FRAMES) { nextState = IDLE; currentFrame = 0; noInputFrames = 0; }
        else { nextState = STANCE; }
    }

    if (currentState != nextState) {
        if (ResetsAnimationOnEnter(nextState)) {
            currentFrame = 0;
            animAccumulator = 0;
            hitThisAttack = false;
        }
        if (nextState == IDLE && IsOnGround()) {
            position.y = CHARACTER_GROUND_Y;
        }
        currentState = nextState;
        frameCounter = 0;
    }

    maxFrame = GetMaxFrameForState(currentState);

    if (stanceEntryDelay <= 0) {
        switch (currentState) {
        case STANCE:
        case SUMMON_1_ORPHEUS:
        case SUMMON_2_JACKFROST:
        case SUMMON_AIR_MAZIODYNE:
            AdvanceLoopFrame(animAccumulator, currentFrame, animSteps, MAKOTO_LOOP_TICKS_SLOW, maxFrame);
            break;
        case WALK:
        case RUN:
            AdvanceLoopFrame(animAccumulator, currentFrame, animSteps, MAKOTO_LOOP_TICKS_FAST, maxFrame);
            break;
        case GUARD:
        case GUARD_AIR:
        case CROUCH:
            AdvanceLoopFrame(animAccumulator, currentFrame, animSteps, MAKOTO_LOOP_TICKS_SLOW, maxFrame);
            break;
        default:
            break;
        }
    }
    UpdateScaledHurtbox();
}

void Makoto::FinishPersonaSequence() {
    int stanceFrames = GetMaxFrameForState(STANCE);
    currentState = STANCE;
    currentFrame = (stanceFrames > 0) ? (currentFrame % stanceFrames) : 0;
    animAccumulator = 0;
    maxFrame = stanceFrames;
    isSuperMoveActive = false;
    isMakotoGray = false;
    overlayColor = 0;
    superMoveTimer = 0;

    isOrpheusActive = false;
    isJackFrostActive = false;
    isAGIActive = false;
    isMabufuActive = false;
    isThanatosActive = false;
    isMaziodyneActive = false;
    isThanatosSlashActive = false;
    isMessiahActive = false;
    isMegidolaonActive = false;

    orpheusFrame = jackfrostFrame = agiFrame = mabufuFrame = 0;
    thanatosFrame = maziodyneFrame = slashFrame = 0;
    messiahFrame = megidolaonFrame = 0;

    orpheusAnimationComplete = jackfrostAnimationComplete = false;
    agiAnimationComplete = mabufuAnimationComplete = false;
    thanatosAnimationComplete = maziodyneAnimationComplete = false;
    thanatosSlashAnimationComplete = messiahAnimationComplete = false;
    megidolaonAnimationComplete = false;

    slashAnimTimer = 0;
    slashTotalFrames = 0;
    personaAnimAccumulator = 0;
}

void Makoto::UpdatePersonaLogic(Joker& enemy, int steps) {
    ApplyGravity(steps);

    if (isSuperMoveActive) {
        superMoveTimer += steps;
        int alpha = superMoveTimer * 4;
        if (alpha > 128) alpha = 128;
        overlayColor = D3DCOLOR_ARGB(alpha, 0, 0, 0);
    }

    if (currentState == SUMMON_1_ORPHEUS || currentState == SUMMON_2_JACKFROST ||
        currentState == SUMMON_AIR_MAZIODYNE || currentState == SUMMON_AIR_MESSIAH ||
        currentState == THANATOS_SLASH) {
        maxFrame = GetMaxFrameForState(STANCE);
        AdvanceLoopFrame(animAccumulator, currentFrame, 1, PERSONA_STANCE_ANIM_TICKS, maxFrame);
    }

    UpdateLiveEffectPositions(enemy);

    const int animDelay =
        (currentState == THANATOS_SLASH) ? THANATOS_SLASH_ANIM_DELAY
        : (currentState == SUMMON_AIR_MAZIODYNE || currentState == SUMMON_AIR_MESSIAH)
        ? PERSONA_EFFECT_ANIM_DELAY
        : PERSONA_ANIM_DELAY;

    personaAnimAccumulator += steps;
    while (personaAnimAccumulator >= animDelay) {
        personaAnimAccumulator -= animDelay;

        switch (currentState) {
        case SUMMON_1_ORPHEUS:
            if (isOrpheusActive && !orpheusAnimationComplete) {
                orpheusFrame++;
                if (orpheusFrame >= makotoOrpheus.maxFrame) {
                    orpheusFrame = makotoOrpheus.maxFrame - 1;
                    orpheusAnimationComplete = true;
                    isOrpheusActive = false;
                    isAGIActive = true;
                    agiFrame = 0;
                }
            }
            else if (isAGIActive && !agiAnimationComplete) {
                agiFrame++;
                if (!hitAGI && agiFrame >= 2 && agiFrame <= makotoAGI.maxFrame - 1) {
                    TrySkillHitOnTarget(enemy, hitAGI, 45);
                }
                if (agiFrame >= makotoAGI.maxFrame) {
                    agiFrame = makotoAGI.maxFrame - 1;
                    agiAnimationComplete = true;
                    isAGIActive = false;
                    FinishPersonaSequence();
                }
            }
            else {
                FinishPersonaSequence();
            }
            break;

        case SUMMON_2_JACKFROST:
            jackfrostPos = D3DXVECTOR3(
                position.x - (float)facingDirection * PERSONA_BEHIND_HORIZONTAL,
                position.y - JACKFROST_BEHIND_VERTICAL,
                0.0f);
            if (isJackFrostActive && !jackfrostAnimationComplete) {
                jackfrostFrame++;
                if (jackfrostFrame >= makotoJackFrost.maxFrame) {
                    jackfrostFrame = makotoJackFrost.maxFrame - 1;
                    jackfrostAnimationComplete = true;
                    isJackFrostActive = false;
                    isMabufuActive = true;
                    mabufuFrame = 0;
                }
            }
            else if (isMabufuActive && !mabufuAnimationComplete) {
                mabufuFrame++;
                if (!hitMabufu && mabufuFrame >= 1 && mabufuFrame <= 3) {
                    TrySkillHitOnTarget(enemy, hitMabufu, 50);
                }
                if (mabufuFrame >= makotoMabufu.maxFrame) {
                    mabufuFrame = makotoMabufu.maxFrame - 1;
                    mabufuAnimationComplete = true;
                    isMabufuActive = false;
                    FinishPersonaSequence();
                }
            }
            else {
                FinishPersonaSequence();
            }
            break;

        case SUMMON_AIR_MAZIODYNE:
            if (isThanatosActive && !thanatosAnimationComplete) {
                thanatosFrame++;
                if (thanatosFrame >= makotoThanatosMaziodyne.maxFrame) {
                    thanatosFrame = makotoThanatosMaziodyne.maxFrame - 1;
                    thanatosAnimationComplete = true;
                    isThanatosActive = false;
                }
            }
            if (isMaziodyneActive && !maziodyneAnimationComplete) {
                maziodyneFrame++;
                if (!hitMaziodyne && maziodyneFrame >= 2 && maziodyneFrame <= 5) {
                    TrySkillHitOnTarget(enemy, hitMaziodyne, 55);
                }
                if (maziodyneFrame >= makotoMaziodyne.maxFrame) {
                    maziodyneFrame = makotoMaziodyne.maxFrame - 1;
                    maziodyneAnimationComplete = true;
                    isMaziodyneActive = false;
                }
            }

            if (thanatosAnimationComplete && maziodyneAnimationComplete) {
                FinishPersonaSequence();
            }
            break;

        case THANATOS_SLASH:
            slashFrame++;
            if (!hitSlash && slashFrame >= 3 && slashFrame <= 6) {
                TrySkillHitOnTarget(enemy, hitSlash, 65);
            }
            if (slashFrame >= makotoThanatosSlash.maxFrame) {
                slashFrame = makotoThanatosSlash.maxFrame - 1;
                thanatosSlashAnimationComplete = true;
                isThanatosSlashActive = false;
                FinishPersonaSequence();
            }
            break;

        case SUMMON_AIR_MESSIAH:
            if (isMessiahActive && !messiahAnimationComplete) {
                messiahFrame++;
                if (messiahFrame >= makotoMessiah.maxFrame) {
                    messiahFrame = makotoMessiah.maxFrame - 1;
                    messiahAnimationComplete = true;
                    isMessiahActive = false;
                }
            }
            if (isMegidolaonActive && !megidolaonAnimationComplete) {
                megidolaonFrame++;
                if (!hitMegidolaon && megidolaonFrame >= 3 && megidolaonFrame <= makotoMegidolaon.maxFrame - 1) {
                    TrySkillHitOnTarget(enemy, hitMegidolaon, 75);
                }
                if (megidolaonFrame >= makotoMegidolaon.maxFrame) {
                    megidolaonFrame = makotoMegidolaon.maxFrame - 1;
                    megidolaonAnimationComplete = true;
                    isMegidolaonActive = false;
                }
            }

            if (messiahAnimationComplete && megidolaonAnimationComplete) {
                FinishPersonaSequence();
            }
            break;

        default:
            FinishPersonaSequence();
            break;
        }
    }
    UpdateScaledHurtbox();
}

void Makoto::Render(LPD3DXSPRITE sprite) {
    if (isDead) return;

    UpdateLiveEffectPositions(g_Player2);

    const MakotoTexture* bodyTex = nullptr;
    int bodyFrame = 0;
    bool drawBody = true;

    if (actionVisualHold > 0) {
        bodyTex = GetTextureForState(actionHoldState);
        if (bodyTex && bodyTex->texture) {
            bodyFrame = ClampMakotoFrameIndex(*bodyTex, actionHoldFrame);
        }
        else {
            drawBody = false;
        }
    }
    else {
        bodyTex = GetTextureForState(currentState);
        if (bodyTex && bodyTex->texture) {
            bodyFrame = ClampMakotoFrameIndex(*bodyTex, currentFrame);
        }
        else {
            drawBody = false;
        }
    }

    D3DCOLOR color = D3DCOLOR_XRGB(255, 255, 255);
    D3DCOLOR fxColor = D3DCOLOR_XRGB(255, 255, 255);

    if (isOrpheusActive && orpheusFrame >= 0 && orpheusFrame < makotoOrpheus.maxFrame) {
        DrawMakotoEffectSprite(sprite, makotoOrpheus, orpheusFrame, orpheusPos, fxColor, facingDirection, PERSONA_EFFECT_SCALE);
    }

    if (drawBody && bodyTex && bodyTex->texture) {
        int renderState = (actionVisualHold > 0) ? actionHoldState : currentState;
        float bodyHeight = MAKOTO_BODY_HEIGHT;
        float feetY = MAKOTO_FEET_Y;
        GetMakotoBodyDrawAnchor(renderState, bodyFrame, bodyHeight, feetY);
        SetMakotoFrameRect(makotoRect, *bodyTex, bodyFrame);
        DrawScaledCharacterSprite(sprite, bodyTex->texture, &makotoRect, position, facingDirection, 1.0f, color, bodyHeight, feetY);
    }

    const int skillFxFacing = GetSkillFacingDirection(position, currentEnemyPos);

    if (isAGIActive && agiFrame >= 0 && agiFrame < makotoAGI.maxFrame) {
        DrawMakotoEffectSprite(sprite, makotoAGI, agiFrame, agiPos, fxColor, skillFxFacing, AGI_EFFECT_SCALE * PERSONA_EFFECT_SCALE);
    }
    if (isJackFrostActive && jackfrostFrame >= 0 && jackfrostFrame < makotoJackFrost.maxFrame) {
        DrawMakotoEffectSprite(sprite, makotoJackFrost, jackfrostFrame, jackfrostPos, fxColor, facingDirection, PERSONA_EFFECT_SCALE);
    }
    if (isMabufuActive && mabufuFrame >= 0 && mabufuFrame < makotoMabufu.maxFrame) {
        DrawMakotoEffectSprite(sprite, makotoMabufu, mabufuFrame, mabufuPos, fxColor, skillFxFacing, MABUFU_EFFECT_SCALE * PERSONA_EFFECT_SCALE);
    }
    if (isThanatosActive && thanatosFrame >= 0 && thanatosFrame < makotoThanatosMaziodyne.maxFrame) {
        DrawMakotoEffectSprite(sprite, makotoThanatosMaziodyne, thanatosFrame, thanatosPos, fxColor, facingDirection, PERSONA_EFFECT_SCALE);
    }
    if (isMaziodyneActive && maziodyneFrame >= 0 && maziodyneFrame < makotoMaziodyne.maxFrame) {
        DrawMakotoEffectSprite(sprite, makotoMaziodyne, maziodyneFrame, maziodynePos, fxColor, facingDirection, PERSONA_EFFECT_SCALE);
    }
    if (isThanatosSlashActive && slashFrame >= 0 && slashFrame < makotoThanatosSlash.maxFrame) {
        DrawMakotoCenteredEffectSprite(sprite, makotoThanatosSlash, slashFrame, slashPos, THANATOS_SLASH_EFFECT_SCALE * PERSONA_EFFECT_SCALE, fxColor);
    }
    if (isMessiahActive && messiahFrame >= 0 && messiahFrame < makotoMessiah.maxFrame) {
        int fxFacing = GetSkillFacingDirection(position, currentEnemyPos);
        DrawMakotoEffectSprite(sprite, makotoMessiah, messiahFrame, messiahPos, fxColor, fxFacing, PERSONA_EFFECT_SCALE);
    }
    if (isMegidolaonActive && megidolaonFrame >= 0 && megidolaonFrame < makotoMegidolaon.maxFrame) {
        int fxFacing = GetSkillFacingDirection(position, currentEnemyPos);
        DrawMakotoEffectSprite(sprite, makotoMegidolaon, megidolaonFrame, megidolaonPos, fxColor, fxFacing, PERSONA_EFFECT_SCALE);
    }

    RenderMeleeHitSpark(sprite);
}

void Makoto::RenderDebugHitbox(LPD3DXSPRITE sprite) {
    if (!sprite) return;
    UpdateScaledHurtbox();
    DrawDebugRect(sprite, hurtbox.x, hurtbox.y, hurtbox.width, hurtbox.height, D3DCOLOR_ARGB(160, 64, 160, 255));
}

void Makoto::TakeDamage(int damage) {
    if (isDead) return;
    health -= damage;
    isHit = true;
    hitStunTimer = 20;
    if (health <= 0) {
        health = 0;
        if (!TRAINING_MODE) {
            isDead = true;
        }
    }
}

void Makoto::Reset() {
    float spawnX = GetMakotoScreenHalfWidth() + MAKOTO_WINDOW_MARGIN + MAKOTO_SPAWN_FORWARD;
    position = D3DXVECTOR3(spawnX, CHARACTER_GROUND_Y, 0);
    facingDirection = 1;
    health = maxHealth;
    sp = maxSp;
    isDead = false;
    isHit = false;
    hitStunTimer = 0;
    currentState = INTRO;
    currentFrame = 0;
    maxFrame = GetMaxFrameForState(INTRO);
    jumpCount = 0;
    verticalVelocity = 0;
    isSuperMoveActive = false;
    isMakotoGray = false;
    overlayColor = 0;
    isOrpheusActive = false; isJackFrostActive = false; isAGIActive = false; isMabufuActive = false;
    isThanatosActive = false; isMaziodyneActive = false; isThanatosSlashActive = false;
    isMessiahActive = false; isMegidolaonActive = false;
    orpheusAnimationComplete = false; jackfrostAnimationComplete = false;
    agiAnimationComplete = false; mabufuAnimationComplete = false;
    thanatosAnimationComplete = false; maziodyneAnimationComplete = false;
    thanatosSlashAnimationComplete = false;
    messiahAnimationComplete = false; megidolaonAnimationComplete = false;
    hitAGI = false; hitMabufu = false; hitMaziodyne = false; hitSlash = false; hitMegidolaon = false;
    hitThisAttack = false; dashHasHit = false;
    spaceChordBuffer = 0; spaceWasDown = false; messiahChordConsumed = false;
    slashAnimTimer = 0;
    slashTotalFrames = 0;
    animAccumulator = 0;
    personaAnimAccumulator = 0;
    noInputFrames = 0;
    introDisplayHold = 0;
    introLastFrame = 0;
    stanceEntryDelay = 0;
    actionVisualHold = 0;
    actionHoldState = STANCE;
    actionHoldFrame = 0;
    meleeHitSparkActive = false;
    meleeHitSparkFrame = 0;
    UpdateScaledHurtbox();
}

bool Makoto::IsAttacking() const {
    return currentState == ATTACK || currentState == SIDE_ATTACK ||
        currentState == ATTACK_UP || currentState == DOWN_ATTACK ||
        currentState == CROUCH_ATTACK;
}

static bool LoadMakotoTexture(MakotoTexture& tex, const char* path, int frameCount) {
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
        D3DCOLOR_XRGB(PERSONA_COLORKEY_R, PERSONA_COLORKEY_G, PERSONA_COLORKEY_B),
        NULL,
        NULL,
        &tex.texture);

    if (FAILED(hr) || tex.texture == NULL) {
        return false;
    }

    ApplyPersonaBlueColorKey(tex.texture);

    D3DSURFACE_DESC desc;
    tex.texture->GetLevelDesc(0, &desc);
    tex.cols = (int)(desc.Width / makotoSpriteWidth);
    tex.rows = (int)(desc.Height / makotoSpriteHeight);
    if (tex.cols < 1) tex.cols = 1;
    if (tex.rows < 1) tex.rows = 1;

    int gridFrames = tex.cols * tex.rows;
    tex.maxFrame = frameCount;
    if (tex.maxFrame <= 0 || tex.maxFrame > gridFrames) {
        tex.maxFrame = gridFrames;
    }
    return true;
}

static void ReleaseMakotoTexture(MakotoTexture& tex) {
    if (tex.texture) {
        tex.texture->Release();
        tex.texture = NULL;
    }
}

bool LoadMakotoTextures() {
    struct TextureLoadInfo {
        MakotoTexture* tex;
        const char* path;
        int frameCount;
    };

    TextureLoadInfo textures[] = {
        { &makotoStance, "assets/makoto/stance.png", 4 },
        { &makotoIdle, "assets/makoto/idle.png", 10 },
        { &makotoWalk, "assets/makoto/walk.png", 7 },
        { &makotoRun, "assets/makoto/run.png", 12 },
        { &makotoDash, "assets/makoto/dash.png", 7 },
        { &makotoJump, "assets/makoto/jump.png", 7 },
        { &makotoAttack, "assets/makoto/attack_combo.png", 14 },
        { &makotoCrouch, "assets/makoto/crouch.png", 2 },
        { &makotoCrouchAtk, "assets/makoto/crouch_attack.png", 5 },
        { &makotoDodge, "assets/makoto/dodge.png", 6 },
        { &makotoGuard, "assets/makoto/guard.png", 3 },
        { &makotoGuardAir, "assets/makoto/guard_air.png", 3 },
        { &makotoSideAttack, "assets/makoto/side_attack.png", 13 },
        { &makotoAttackUp, "assets/makoto/attack_up.png", 6 },
        { &makotoDownAttack, "assets/makoto/down_attack.png", 10 },
        { &makotoNeutralAir, "assets/makoto/neutral_air.png", 4 },
        { &makotoUpAir, "assets/makoto/up_air.png", 7 },
        { &makotoSideAir, "assets/makoto/side_air.png", 6 },
        { &makotoDownAir, "assets/makoto/down_air.png", 8 },
        { &makotoIntro, "assets/makoto/intro.png", 8 },
        { &makotoTaunt, "assets/makoto/taunt.png", 8 },
        { &makotoDamage, "assets/makoto/damage.png", 3 },
        { &makotoRecover, "assets/makoto/recover.png", 4 },
        { &makotoSummon1, "assets/makoto/persona_summon_1.png", 14 },
        { &makotoSummon2, "assets/makoto/persona_summon_2.png", 19 },
        { &makotoSummonAir, "assets/makoto/persona_summonair_1.png", 12 },
        { &makotoSummonAir2, "assets/makoto/persona_summonair_2.png", 18 },
        { &makotoOrpheus, "assets/makoto/orpheus.png", 10 },
        { &makotoJackFrost, "assets/makoto/jackfrost.png", 12 },
        { &makotoAGI, "assets/makoto/agi.png", 12 },
        { &makotoMabufu, "assets/makoto/mabufu.png", 4 },
        { &makotoMaziodyne, "assets/makoto/maziodyne.png", 6 },
        { &makotoThanatosMaziodyne, "assets/makoto/thanatos_maziodyne.png", 10 },
        { &makotoThanatosSlash, "assets/makoto/thanatos_slash.png", 9 },
        { &makotoMessiah, "assets/makoto/messiah.png", 6 },
        { &makotoMegidolaon, "assets/makoto/megidolaon.png", 15 },
        { &makotoWinTex, "assets/makoto/makoto_win.png", 12 },
        { &makotoThanatosWin, "assets/makoto/thanatos_win.png", 12 },
    };

    for (int i = 0; i < (int)(sizeof(textures) / sizeof(textures[0])); ++i) {
        if (!LoadMakotoTexture(*textures[i].tex, textures[i].path, textures[i].frameCount)) {
            char msg[512];
            sprintf_s(msg, "Failed to load %s", textures[i].path);
            MessageBox(g_hWnd, msg, "Makoto Texture Error", MB_OK);
            return false;
        }
    }

    g_MessiahSheetBounds.maxFrameWidth = (float)makotoSpriteWidth;
    g_MessiahSheetBounds.maxFrameHeight = (float)makotoSpriteHeight;
    g_MegidolaonBurstBounds.maxFrameWidth = (float)makotoSpriteWidth;
    g_MegidolaonBurstBounds.maxFrameHeight = (float)makotoSpriteHeight;
    g_MegidolaonBlastBounds.maxFrameWidth = (float)makotoSpriteWidth;
    g_MegidolaonBlastBounds.maxFrameHeight = (float)makotoSpriteHeight;

    return true;
}

void CleanUpMakotoTextures() {
    ReleaseMakotoTexture(makotoStance);
    ReleaseMakotoTexture(makotoIdle);
    ReleaseMakotoTexture(makotoWalk);
    ReleaseMakotoTexture(makotoRun);
    ReleaseMakotoTexture(makotoDash);
    ReleaseMakotoTexture(makotoJump);
    ReleaseMakotoTexture(makotoAttack);
    ReleaseMakotoTexture(makotoCrouch);
    ReleaseMakotoTexture(makotoCrouchAtk);
    ReleaseMakotoTexture(makotoDodge);
    ReleaseMakotoTexture(makotoGuard);
    ReleaseMakotoTexture(makotoGuardAir);
    ReleaseMakotoTexture(makotoSideAttack);
    ReleaseMakotoTexture(makotoAttackUp);
    ReleaseMakotoTexture(makotoDownAttack);
    ReleaseMakotoTexture(makotoNeutralAir);
    ReleaseMakotoTexture(makotoUpAir);
    ReleaseMakotoTexture(makotoSideAir);
    ReleaseMakotoTexture(makotoDownAir);
    ReleaseMakotoTexture(makotoIntro);
    ReleaseMakotoTexture(makotoTaunt);
    ReleaseMakotoTexture(makotoDamage);
    ReleaseMakotoTexture(makotoRecover);
    ReleaseMakotoTexture(makotoSummon1);
    ReleaseMakotoTexture(makotoSummon2);
    ReleaseMakotoTexture(makotoSummonAir);
    ReleaseMakotoTexture(makotoSummonAir2);
    ReleaseMakotoTexture(makotoOrpheus);
    ReleaseMakotoTexture(makotoJackFrost);
    ReleaseMakotoTexture(makotoAGI);
    ReleaseMakotoTexture(makotoMabufu);
    ReleaseMakotoTexture(makotoMaziodyne);
    ReleaseMakotoTexture(makotoThanatosMaziodyne);
    ReleaseMakotoTexture(makotoThanatosSlash);
    ReleaseMakotoTexture(makotoMessiah);
    ReleaseMakotoTexture(makotoMegidolaon);
    ReleaseMakotoTexture(makotoWinTex);
    ReleaseMakotoTexture(makotoThanatosWin);
}