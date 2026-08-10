#pragma once
#include "config.h"

// Base fighter for the OO game framework (BMCS2224).
// Makoto / Joker inherit this and override Update / Render / combat hooks.
class Fighter {
public:
    D3DXVECTOR3 position;
    int facingDirection;
    int health;
    int maxHealth;
    int sp;
    int maxSp;
    AABB hurtbox;
    bool isHit;
    int hitStunTimer;
    bool isDead;
    int velocity;

    Fighter();
    virtual ~Fighter() {}

    virtual void Update() = 0;
    virtual void Render(LPD3DXSPRITE sprite) = 0;
    virtual void TakeDamage(int damage) = 0;
    virtual void ApplySkillDamage(int damage) { TakeDamage(damage); }
    virtual void Reset() = 0;

    int GetHealth() const { return health; }
    int GetMaxHealth() const { return maxHealth; }
    int GetSp() const { return sp; }
    int GetMaxSp() const { return maxSp; }

    bool TryConsumeSp(int cost);
    void RestoreSp(int amount);

    AABB GetHurtbox() const { return hurtbox; }
    void UpdateHurtbox(float offsetX, float offsetY, float width, float height);
    // Rebuild hurtbox from DEFAULT_HURTBOX_* constants and current position.
    void UpdateScaledHurtbox();
};