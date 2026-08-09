#pragma once
#include "../Fighter.h"
#include "JokerAssets.h"

struct JokerTextureSet;

enum JokerState {
    JOKER_STAND,
    JOKER_WALK,
    JOKER_DAMAGE,
    JOKER_DEAD
};

class Joker : public Fighter {
private:
    int currentFrame;
    int maxFrame;
    int frameCounter;
    int currentState;
    bool isStunned;
    bool isReturningToPosition;
    D3DXVECTOR3 originalPosition;
    int damageAnimLength;
    int damageTimer;
    int stunTimer;
    bool isDamageAnimating;
    bool isForceResetting;
    bool bForceReset;
    bool shouldReturnToOriginal;
    bool isActive;
    int trainingIdleFrames;

    void UpdateHurtbox();
    void TryTrainingHeal();
    void BeginHitReaction(float knockbackX);
    void ReturnToOriginalPosition();
    void ResetAllStates();
    void ClampPosition();
    void DrawBodySprite(LPD3DXSPRITE sprite, struct JokerTexture& tex, int frame, const D3DXVECTOR3& pos, D3DCOLOR color) const;
    void DrawArseneSprite(LPD3DXSPRITE sprite, struct JokerTexture& tex, int frame, D3DCOLOR color) const;
    void DrawEffectSprite(LPD3DXSPRITE sprite, struct JokerTexture& tex, int frame, const D3DXVECTOR3& pos, float bodyHeight, float feetY, D3DCOLOR color) const;

public:
    Joker();
    ~Joker() override = default;

    void Update() override;
    void Render(LPD3DXSPRITE sprite) override;
    void TakeDamage(int damage) override;
    void ApplySkillDamage(int damage) override;
    void Reset() override;

    void RenderDebugHitbox(LPD3DXSPRITE sprite);

    bool IsDead() const { return isDead; }
    bool IsHit() const { return isHit; }
    const D3DXVECTOR3& GetPosition() const { return position; }
    int GetFacingDirection() const { return facingDirection; }
    AABB GetHurtbox();
    AABB GetBodyCollisionBox();
};

bool LoadJokerTextures();
void CleanUpJokerTextures();

const struct JokerTextureSet* GetJokerAnimSet(JokerAnimId id);
