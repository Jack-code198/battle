#pragma once
#include "config.h"
#include "CharacterId.h"

// Base fighter for the OO game framework (BMCS2224).
// Makoto / Joker / Narukami inherit this and override Update / Render / combat hooks.
class Fighter {
protected:
    CharacterId characterId;
    int playerSlot;          // 0 = P1 (left), 1 = P2 (right)
    bool humanControlled;    // true = human input; false = sandbag

public:
    D3DXVECTOR3 position;
    int facingDirection;
    int health;
    int maxHealth;
    int sp;
    int maxSp;
    float stamina;
    float maxStamina;
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

    CharacterId GetCharacterId() const { return characterId; }
    int GetPlayerSlot() const { return playerSlot; }
    bool IsHumanControlled() const { return humanControlled; }
    void SetHumanControlled(bool human) { humanControlled = human; }
    void SetPlayerSlot(int slot) { playerSlot = slot; }
    void SetCharacterId(CharacterId id) { characterId = id; }

    // P1 owns the shared FrameTimer; P2 reads last step.
    bool OwnsFrameTimer() const { return playerSlot == 0; }

    virtual const char* GetDisplayName() const { return GetCharacterDisplayName(characterId); }
    virtual const char* GetIconPath() const { return GetCharacterIconPath(characterId); }
    virtual bool IsSuperMoveActive() const { return false; }
    virtual D3DCOLOR GetOverlayColor() const { return 0; }
    virtual void RenderDebugHitbox(LPD3DXSPRITE sprite) {}

    int GetHealth() const { return health; }
    int GetMaxHealth() const { return maxHealth; }
    int GetSp() const { return sp; }
    int GetMaxSp() const { return maxSp; }
    float GetStamina() const { return stamina; }
    float GetMaxStamina() const { return maxStamina; }
    bool IsDead() const { return isDead; }
    bool IsHit() const { return isHit; }

    bool TryConsumeSp(int cost);
    void RestoreSp(int amount);
    bool HasStamina(float cost) const;
    bool TryConsumeStamina(float cost);
    void DrainStamina(float amount);
    void RestoreStamina(float amount);
    void RefillStamina();
    // While sprinting: gradual drain. Returns false if stamina ran out (caller should force walk).
    bool DrainStaminaWhileRunning(int animSteps);
    // Passiveive refill while not sprinting.
    void RegenStamina(int animSteps);

    AABB GetHurtbox() const { return hurtbox; }
    // Push / body overlap box. Default = hurtbox; characters may override.
    virtual AABB GetBodyCollisionBox() const { return hurtbox; }
    const D3DXVECTOR3& GetPosition() const { return position; }
    int GetFacingDirection() const { return facingDirection; }

    void UpdateHurtbox(float offsetX, float offsetY, float width, float height);
    // Rebuild hurtbox from DEFAULT_HURTBOX_* constants and current position.
    virtual void UpdateScaledHurtbox();

    // Slot-aware spawn helpers used by Reset implementations.
    void ApplySlotSpawnDefaults();
};
