#include "Fighter.h"

Fighter::Fighter()
    : facingDirection(1)
    , health(MAKOTO_MAX_HEALTH)
    , maxHealth(MAKOTO_MAX_HEALTH)
    , sp(FIGHTER_MAX_SP)
    , maxSp(FIGHTER_MAX_SP)
    , isHit(false)
    , hitStunTimer(0)
    , isDead(false)
    , velocity(4) {
    position = D3DXVECTOR3(0, 0, 0);
    hurtbox = { 0, 0, 0, 0 };
}

void Fighter::UpdateHurtbox(float offsetX, float offsetY, float width, float height) {
    hurtbox.x = position.x + offsetX;
    hurtbox.y = position.y + offsetY;
    hurtbox.width = width;
    hurtbox.height = height;
}

void Fighter::UpdateScaledHurtbox() {
    float s = GetCharacterRenderScale();
    hurtbox.width = 36.0f * s;
    hurtbox.height = 110.0f * s;
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