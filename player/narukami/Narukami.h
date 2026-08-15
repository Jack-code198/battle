#pragma once
#include "../Fighter.h"
#include "../../config.h"

// Yu Narukami fighter (OO subclass of Fighter).
// Compact Makoto-style locomotion/melee with Zio / Ziodyne persona skills.

enum NarukamiState {
    NARUKAMI_INTRO,
    NARUKAMI_INTRO_DISCARD,   // intro_effect: throw sword hilt after intro
    NARUKAMI_STANCE,
    NARUKAMI_IDLE,
    NARUKAMI_WALK,
    NARUKAMI_RUN,
    NARUKAMI_DASH,
    NARUKAMI_JUMP,
    NARUKAMI_PERSONA_SUMMON,      // persona_attack → then izanagi_attack
    NARUKAMI_PERSONA_AIR_SUMMON,  // persona_air_attack → then izanagi_air_attack
    NARUKAMI_ATTACK,
    NARUKAMI_CROUCH,
    NARUKAMI_CROUCH_ATTACK,
    NARUKAMI_GUARD,
    NARUKAMI_SIDE_ATTACK,
    NARUKAMI_ATTACK_UP,
    NARUKAMI_DOWN_ATTACK,
    NARUKAMI_NEUTRAL_AIR,
    NARUKAMI_DAMAGE,
    NARUKAMI_RECOVER,
    NARUKAMI_TAUNT,
    NARUKAMI_SUMMON_ZIO,
    NARUKAMI_SUMMON_ZIODYNE,
    NARUKAMI_RAGING_LION,
    NARUKAMI_BIG_GAMBLE,
    NARUKAMI_MYRIAD_TRUTHS,
    NARUKAMI_WIN,
    NARUKAMI_LOSE
};

class Narukami : public Fighter {
private:
    int currentFrame;
    int maxFrame;
    int animAccumulator;
    int currentState;

    int jumpCount;
    float jumpHorizontalSpeed;
    float verticalVelocity;

    bool hitThisAttack;
    bool dashHasHit;
    bool attackButtonHeld;
    bool skillHit;
    bool jumpSpaceWasReleased;

    int personaAnimAccumulator;
    int izanagiFrame;
    int effectFrame;
    bool showIzanagi;
    bool showEffect;
    D3DXVECTOR3 izanagiPos;
    D3DXVECTOR3 effectPos;
    D3DXVECTOR3 discardPos;
    bool discardFlying;

    int noInputFrames;
    int idleWaitFrames;
    int damageGroundHold;
    int introDisplayHold;
    int introLastFrame;
    int pendingAttackState; // attack state after persona summon finishes
    D3DXVECTOR3 spawnPosition;

    void EnterState(int state);
    void CompleteToStance();
    void BeginIntroDiscard();
    void BeginPersonaSummon(int summonState, int followUpAttack);
    void UpdateHuman(int steps);
    void UpdateSandbag(int steps);
    void UpdateSummon(int steps, Fighter& enemy);
    void ApplyGravity(int steps);
    bool IsOnGround() const;
    void CheckAttackCollision(Fighter& enemy);
    void BeginHitReaction();
    void BeginRecover();
    void FinishRecover();
    void BeginSummon(int state);

public:
    Narukami();
    ~Narukami() override = default;

    void Update() override;
    void Render(LPD3DXSPRITE sprite) override;
    void RenderSkillBackdropBeforeOpponent(LPD3DXSPRITE sprite) override;
    void TakeDamage(int damage) override;
    void ApplySkillDamage(int damage) override;
    void Reset() override;

    void BeginVictoryPose() override;
    void BeginDefeatPose() override;
    bool IsPlayingResultPose() const override;
    bool IsInCombatAction() const override;

    bool IsSuperMoveActive() const override;
    D3DCOLOR GetOverlayColor() const override;
    void RenderDebugHitbox(LPD3DXSPRITE sprite) override;
    AABB GetBodyCollisionBox() const override;
    void UpdateScaledHurtbox() override;

    int GetCurrentState() const { return currentState; }
};

bool LoadNarukamiTextures();
void CleanUpNarukamiTextures();
