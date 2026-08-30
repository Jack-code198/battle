#pragma once
#include "../Fighter.h"
#include "../../Config.h"

// Yosuke Hanamura fighter (OO subclass of Fighter).
// Makoto-style locomotion with Jiraiya persona skills.

enum YosukeState {
    YOSUKE_INTRO,
    YOSUKE_STANCE,
    YOSUKE_IDLE,
    YOSUKE_WALK,
    YOSUKE_RUN,
    YOSUKE_DASH,
    YOSUKE_BACK_DASH,
    YOSUKE_JUMP,
    YOSUKE_GUARD,
    YOSUKE_GUARD_AIR,
    YOSUKE_ATTACK,
    YOSUKE_AIR_COMBO,
    YOSUKE_CRESCENT_SLASH,
    YOSUKE_MOONSAULT,
    YOSUKE_FLYING_KUNAI,
    YOSUKE_PERSONA_SUMMON,
    YOSUKE_PERSONA_JIRAIYA,
    YOSUKE_MIRAGE_SLASH,
    YOSUKE_BRAVE_BLADE,
    YOSUKE_GARUDYNE,
    YOSUKE_DAMAGE,
    YOSUKE_RECOVER,
    YOSUKE_WIN,
    YOSUKE_LOSE
};

class Yosuke : public Fighter {
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
    int jiraiyaFrame;
    int effectFrame;
    bool showJiraiya;
    bool showEffect;
    D3DXVECTOR3 jiraiyaPos;
    D3DXVECTOR3 effectPos;
    D3DXVECTOR3 flyingKunaiTargetPos;

    int noInputFrames;
    int idleWaitFrames;
    int damageGroundHold;
    int introDisplayHold;
    int introLastFrame;
    int spaceChordBuffer;
    bool spaceWasDown;
    bool crescentButtonHeld;
    bool personaKey1Held;
    bool personaKey2Held;
    bool personaKey3Held;
    bool personaKey4Held;
    bool moonsaultButtonHeld;

    void EnterState(int state);
    void CompleteToStance();
    void BeginPersonaSummon();
    void BeginGarudyne();
    void BeginFlyingKunai(Fighter* opponent);
    void UpdateHuman(int steps);
    void UpdateSandbag(int steps);
    void UpdateIntro(int steps);
    void UpdateGarudyne(int steps, Fighter& enemy);
    void UpdateIntroDropPosition();
    void ApplyGravity(int steps);
    bool IsOnGround() const;
    void CheckAttackCollision(Fighter& enemy);
    void BeginHitReaction();
    void BeginRecover();
    void FinishRecover();
    bool CanUseSpaceChord() const;
    void TickSpaceChordBuffer(bool isJumpPressed, int steps);
    bool IsHoldingAwayInput() const;
    void UpdateLiveSkillTargets(Fighter& enemy);
    void UpdateFlyingKunaiProjectile(Fighter& enemy, int steps);

public:
    Yosuke();
    ~Yosuke() override = default;

    void Update() override;
    void SyncHeldInputState() override;
    void Render(LPD3DXSPRITE sprite) override;
    void RenderSkillBackdropBeforeOpponent(LPD3DXSPRITE sprite) override;
    void TakeDamage(int damage) override;
    void ApplySkillDamage(int damage) override;
    void Reset() override;

    void BeginVictoryPose() override;
    void BeginDefeatPose() override;
    bool IsPlayingResultPose() const override;
    bool IsInKnockdownReaction() const override;
    void SnapStandForUltimate();
    bool IsInCombatAction() const override;
    bool IsInGuardState() const override;
    void HoldGuardState(bool airborne) override;

    bool IsSuperMoveActive() const override;
    D3DCOLOR GetOverlayColor() const override;
    void RenderDebugHitbox(LPD3DXSPRITE sprite) override;
    AABB GetBodyCollisionBox() const override;
    void UpdateScaledHurtbox() override;

    int GetCurrentState() const { return currentState; }
    int GetActionState() const override { return currentState; }
};

bool LoadYosukeTextures();
void CleanUpYosukeTextures();
