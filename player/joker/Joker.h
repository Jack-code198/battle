#pragma once
#include "../Fighter.h"
#include "JokerAssets.h"

// Joker fighter (OO subclass of Fighter).
// Supports sandbag training mode and human P1 combat with Arsene pairing.

struct JokerTextureSet;

enum JokerState {
    JOKER_INTRO,
    JOKER_STAND,
    JOKER_IDLE,
    JOKER_WALK,
    JOKER_RUN,
    JOKER_DASH,
    JOKER_JUMP,
    JOKER_GUARD,
    JOKER_GUARD_AIR,
    JOKER_ATTACK,
    JOKER_FORWARD_ATTACK,
    JOKER_UP_ATTACK,
    JOKER_DOWN_ATTACK,
    JOKER_FORWARD_SMASH,
    JOKER_UP_SMASH,
    JOKER_DOWN_SMASH,
    JOKER_NEUTRAL_AIR,
    JOKER_FORWARD_AIR,
    JOKER_BACK_AIR,
    JOKER_DOWN_AIR,
    JOKER_UP_AIR,
    JOKER_NEUTRAL_SPECIAL,
    JOKER_NEUTRAL_AIR_SPECIAL,
    JOKER_EIHA,
    JOKER_EIGAON,
    JOKER_ALL_OUT_ATTACK,
    JOKER_ALL_OUT_MEMBER,
    JOKER_ALL_OUT_EFFECT,
    JOKER_ALL_OUT_FINISH,
    JOKER_PERSONA_SUMMON,
    JOKER_PERSONA_RETURN,
    JOKER_DODGE,
    JOKER_LEDGEROLL,
    JOKER_TAUNT,
    JOKER_DAMAGE,
    JOKER_RECOVER,
    JOKER_WIN,
    JOKER_LOSE,
    JOKER_DEAD
};

class Joker : public Fighter {
private:
    int currentFrame;
    int maxFrame;
    int frameCounter;
    int animAccumulator;
    int currentState;
    bool isStunned;
    bool isReturningToPosition;
    D3DXVECTOR3 originalPosition;
    int damageAnimLength;
    int damageTimer;
    int stunTimer;
    bool isDamageAnimating;
    bool isForceResetting;
    bool forceResetRequested;
    bool shouldReturnToOriginal;
    bool isActive;
    int trainingIdleFrames;
    int idleWaitFrames;
    int damageGroundHold;
    int introDisplayHold;
    int introLastFrame;

    int jumpCount;
    float jumpHorizontalSpeed;
    float verticalVelocity;
    bool hitThisAttack;
    bool attackButtonHeld;
    bool dodgeForward;
    bool skillHit;
    int skillEffectFrame;
    int skillEffectAccum;
    D3DXVECTOR3 skillEffectPos;
    int pendingSkillState;
    int personaAnimAccumulator;
    int arseneSkillFrame;
    bool showArseneSkill;

    bool winRunoffActive;

    void UpdateHurtbox();
    void TryTrainingHeal();
    void BeginHitReaction(float knockbackX);
    void BeginRecover();
    void FinishRecoverToStance();
    void ReturnToOriginalPosition();
    void ResetAllStates();
    void EnterStance();
    void EnterIdle();
    void EnterActionState(int state);
    void ClampPosition();
    void UpdateSandbag(int steps);
    void UpdateHuman(int steps);
    void CheckAttackCollision(Fighter& enemy);
    void UpdateSkillHits(Fighter& enemy, int steps);
    void BeginPersonaSummonIntro(int pendingSkill);
    void BeginEihaEigaonSkill(int state);
    void UpdateEihaEigaonSkill(Fighter& enemy, int steps);
    void ApplyGravity(int steps);
    bool IsOnGround() const;
    void DrawBodySprite(LPD3DXSPRITE sprite, struct JokerTexture& tex, int frame, const D3DXVECTOR3& pos, D3DCOLOR color) const;
    void DrawArseneSprite(LPD3DXSPRITE sprite, struct JokerTexture& tex, int frame, D3DCOLOR color) const;
    void DrawEffectSprite(LPD3DXSPRITE sprite, struct JokerTexture& tex, int frame, const D3DXVECTOR3& pos, float bodyHeight, float feetY, D3DCOLOR color) const;
    void DrawSkillEffectOnOpponent(LPD3DXSPRITE sprite, struct JokerTexture& tex, int frame, D3DCOLOR color) const;

public:
    Joker();
    ~Joker() override = default;

    void Update() override;
    void SyncHeldInputState() override;
    void Render(LPD3DXSPRITE sprite) override;
    void TakeDamage(int damage) override;
    void ApplySkillDamage(int damage) override;
    void Reset() override;

    void BeginVictoryPose() override;
    void BeginDefeatPose() override;
    bool IsPlayingResultPose() const override;
    bool IsInKnockdownReaction() const override;
    bool IsInCombatAction() const override;
    bool IsInGuardState() const override;
    void HoldGuardState(bool airborne) override;

    void RenderDebugHitbox(LPD3DXSPRITE sprite) override;

    bool IsSuperMoveActive() const override;
    D3DCOLOR GetOverlayColor() const override;

    AABB GetHurtbox();
    AABB GetBodyCollisionBox() const override;
    void UpdateScaledHurtbox() override;

    int GetActionState() const override { return currentState; }
};

bool LoadJokerTextures();
void CleanUpJokerTextures();

const struct JokerTextureSet* GetJokerAnimSet(JokerAnimId id);
