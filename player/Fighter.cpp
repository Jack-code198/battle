#include "Fighter.h"

Fighter::Fighter()
    : characterId(Char_Makoto)
    , playerSlot(0)
    , humanControlled(true)
    , physicsBody()
    , facingDirection(1)
    , health(MAKOTO_MAX_HEALTH)
    , maxHealth(MAKOTO_MAX_HEALTH)
    , sp(FIGHTER_MAX_SP)
    , maxSp(FIGHTER_MAX_SP)
    , stamina(FIGHTER_MAX_STAMINA)
    , maxStamina(FIGHTER_MAX_STAMINA)
    , isHit(false)
    , hitStunTimer(0)
    , isDead(false)
    , velocity(JOKER_MOVE_SPEED) {
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
            GRAVITY,
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

bool Fighter::TryConsumeSp(int cost) {
    if (cost <= 0) return true;
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

bool Fighter::DrainStaminaWhileRunning(int animSteps) {
    if (animSteps <= 0) return stamina > 0.0f;
    DrainStamina(STAMINA_RUN_DRAIN_PER_STEP * (float)animSteps);
    return stamina > 0.0f;
}

void Fighter::RegenStamina(int animSteps) {
    if (animSteps <= 0) return;
    RestoreStamina(STAMINA_REGEN_PER_STEP * (float)animSteps);
}

void Fighter::ApplySlotSpawnDefaults() {
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
