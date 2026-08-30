#include "Fighter.h"
#include "../GameLogic.h"

Fighter::Fighter()
    : characterId(Char_Makoto)
    , playerSlot(0)
    , humanControlled(true)
    , physicsBody()
    , aiAttackCooldown(0)
    , aiJumpCooldown(0)
    , aiAttackPulse(0)
    , aiAttackMode(0)
    , aiMovePulse(0)
    , aiMoveIntent(0)
    , skillEndHold(0)
    , runBlend(0.0f)
    , facingDirection(1)
    , health(MAKOTO_MAX_HEALTH)
    , maxHealth(MAKOTO_MAX_HEALTH)
    , sp(0)
    , maxSp(FIGHTER_MAX_SP)
    , stamina(FIGHTER_MAX_STAMINA)
    , maxStamina(FIGHTER_MAX_STAMINA)
    , isHit(false)
    , hitStunTimer(0)
    , isDead(false)
    , velocity(JOKER_MOVE_SPEED)
    , resultPoseAnimLocked(false)
    , resultPoseHoldFrame(-1)
    , spriteTint(D3DCOLOR_XRGB(255, 255, 255)) {
    position = D3DXVECTOR3(0, 0, 0);
    hurtbox = { 0, 0, 0, 0 };
    physicsBody.mass = 1.0f;
    physicsBody.position = position;
}

// Shared gravity path for all fighters (BMCS2224 Physics module).
// Uses force-based integration so jump/fall is not duplicated per character.
void Fighter::ApplyPhysicsGravitySteps(int steps, float& verticalVelocityIO) {
    physicsBody.position = position;
    physicsBody.velocity.x = 0.0f;
    physicsBody.velocity.y = verticalVelocityIO;
    physicsBody.ClearForce();

    for (int step = 0; step < steps; ++step) {
        PhysicsWorld::IntegrateGravityOnGround(
            physicsBody,
            CHARACTER_GROUND_Y,
            GRAVITY * BATTLE_GAMEPLAY_SPEED,
            1.0f);
    }

    position.y = physicsBody.position.y;
    verticalVelocityIO = physicsBody.GetVerticalVelocity();
}

void Fighter::UpdateHurtbox(float offsetX, float offsetY, float width, float height) {
    hurtbox.x = position.x + offsetX;
    hurtbox.y = position.y + offsetY;
    hurtbox.width = width;
    hurtbox.height = height;
}

void Fighter::UpdateScaledHurtbox() {
    // Convert unscaled body-unit hurtbox into screen space around fighter feet/center.
    float renderScale = GetCharacterRenderScale();
    hurtbox.width = DEFAULT_HURTBOX_WIDTH * renderScale;
    hurtbox.height = DEFAULT_HURTBOX_HEIGHT * renderScale;
    hurtbox.x = position.x - hurtbox.width * 0.5f;
    hurtbox.y = position.y - hurtbox.height;
}

bool Fighter::CanReceiveHit() const {
    if (isDead || IsPlayingResultPose()) {
        return false;
    }
    // Tutorial sandbag stays hittable so training combos / skills always register.
    if (IsTutorialSandbagMode() && !humanControlled) {
        return true;
    }
    return !IsInKnockdownReaction();
}

void Fighter::TryApplyHorizontalDelta(float deltaX) {
    if (deltaX == 0.0f) return;
    deltaX *= BATTLE_GAMEPLAY_SPEED;

    Fighter* opponent = GetOpponent(*this);
    if (!opponent || opponent->IsDead()) {
        position.x += deltaX;
        return;
    }

    float remaining = deltaX;
    while (fabsf(remaining) > 0.001f) {
        const float step = (fabsf(remaining) > BODY_MOVE_SUBSTEP)
            ? (remaining > 0.0f ? BODY_MOVE_SUBSTEP : -BODY_MOVE_SUBSTEP)
            : remaining;
        position.x += step;
        ClampFighterAgainstOpponent(*this, *opponent);
        remaining -= step;
    }
}

bool Fighter::TryConsumeSp(int cost) {
    if (cost <= 0) return true;
    if (IsTutorialBattleMode() && IsTutorialInfiniteHpSpEnabled()) {
        sp -= cost;
        if (sp < 0) sp = 0;
        return true;
    }
    if (sp < cost) return false;
    sp -= cost;
    return true;
}

void Fighter::RestoreSp(int amount) {
    if (amount <= 0) return;
    sp += amount;
    if (sp > maxSp) sp = maxSp;
}

bool Fighter::HasStamina(float cost) const {
    if (cost <= 0.0f) return true;
    return stamina + 0.0001f >= cost;
}

bool Fighter::TryConsumeStamina(float cost) {
    if (cost <= 0.0f) return true;
    if (!HasStamina(cost)) return false;
    stamina -= cost;
    if (stamina < 0.0f) stamina = 0.0f;
    return true;
}

void Fighter::DrainStamina(float amount) {
    if (amount <= 0.0f) return;
    stamina -= amount;
    if (stamina < 0.0f) stamina = 0.0f;
}

void Fighter::RestoreStamina(float amount) {
    if (amount <= 0.0f) return;
    stamina += amount;
    if (stamina > maxStamina) stamina = maxStamina;
}

void Fighter::RefillStamina() {
    stamina = maxStamina;
}

void Fighter::ResetAiState() {
    aiAttackCooldown = 0;
    aiJumpCooldown = 0;
    aiAttackPulse = 0;
    aiAttackMode = 0;
    aiMovePulse = 0;
    aiMoveIntent = 0;
}

bool Fighter::DrainStaminaWhileRunning(int animSteps) {
    if (animSteps <= 0) return stamina > 0.0f;
    DrainStamina(STAMINA_RUN_DRAIN_PER_STEP * (float)animSteps);
    return stamina > 0.0f;
}

void Fighter::RegenStamina(int animSteps) {
    if (animSteps <= 0) return;
    float regenRate = STAMINA_REGEN_PER_STEP;
    if (IsTutorialBattleMode() && IsTutorialInfiniteHpSpEnabled()) {
        regenRate *= TUTORIAL_STAMINA_REGEN_MULTIPLIER;
    }
    RestoreStamina(regenRate * (float)animSteps);
}

void Fighter::ApplySlotSpawnDefaults() {
    runBlend = 0.0f;
    if (playerSlot == 0) {
        float spawnX = GetMakotoScreenHalfWidth() + MAKOTO_WINDOW_MARGIN + MAKOTO_SPAWN_FORWARD;
        position = D3DXVECTOR3(spawnX, CHARACTER_GROUND_Y, 0);
        facingDirection = 1;
    }
    else {
        position = D3DXVECTOR3(JOKER_SPAWN_X, CHARACTER_GROUND_Y, 0);
        facingDirection = -1;
    }
}

D3DCOLOR Fighter::ApplySpriteTint(D3DCOLOR base, D3DCOLOR tint) {
    if (tint == D3DCOLOR_XRGB(255, 255, 255)) return base;
    const int br = (base >> 16) & 0xFF;
    const int bg = (base >> 8) & 0xFF;
    const int bb = base & 0xFF;
    const int tr = (tint >> 16) & 0xFF;
    const int tg = (tint >> 8) & 0xFF;
    const int tb = tint & 0xFF;
    return D3DCOLOR_XRGB(
        (br * tr) / 255,
        (bg * tg) / 255,
        (bb * tb) / 255);
}
