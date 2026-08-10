#pragma once
#include "../Fighter.h"
#include "../../config.h"

// Player 1 fighter (OO subclass of Fighter).
// Owns Makoto animation state, melee/persona skills, gravity jump, and sprite rendering.

enum MakotoState {
    STANCE, IDLE, WALK, RUN, DASH, JUMP,
    ATTACK, CROUCH, CROUCH_ATTACK, DODGE_FORWARD, DODGE_BACKWARD, GUARD, GUARD_AIR,
    SIDE_ATTACK, ATTACK_UP, DOWN_ATTACK,
    NEUTRAL_AIR, UP_AIR, SIDE_AIR, DOWN_AIR,
    INTRO, TAUNT, DAMAGE, RECOVER,
    SUMMON_1, SUMMON_2, SUMMON_AIR, SUMMON_AIR_2,
    SUMMON_1_ORPHEUS, SUMMON_2_JACKFROST,
    SUMMON_AIR_THANATOS,
    SUMMON_AIR_MAZIODYNE,
    THANATOS_SLASH,
    SUMMON_AIR_MESSIAH,
    MAKOTO_WIN,
    THANATOS_WIN
};

class Makoto : public Fighter {
private:
    int currentFrame;
    int maxFrame;
    int frameCounter;
    int currentState;

    int jumpCount;
    float jumpHorizontalSpeed;
    float verticalVelocity;

    bool isSuperMoveActive;
    bool isMakotoGray;
    int superMoveTimer;
    D3DCOLOR overlayColor;

    D3DXVECTOR3 currentEnemyPos;

    bool isOrpheusActive, isJackFrostActive, isAGIActive, isMabufuActive;
    bool isThanatosActive, isMaziodyneActive, isThanatosSlashActive;
    bool isMessiahActive, isMegidolaonActive;

    bool orpheusAnimationComplete, jackfrostAnimationComplete, agiAnimationComplete, mabufuAnimationComplete;
    bool thanatosAnimationComplete, maziodyneAnimationComplete, thanatosSlashAnimationComplete;
    bool messiahAnimationComplete, megidolaonAnimationComplete;

    int orpheusFrame, jackfrostFrame, agiFrame, mabufuFrame;
    int thanatosFrame, maziodyneFrame, slashFrame;
    int messiahFrame, megidolaonFrame;

    D3DXVECTOR3 orpheusPos, jackfrostPos, agiPos, mabufuPos;
    D3DXVECTOR3 thanatosPos, maziodynePos, slashPos;
    D3DXVECTOR3 messiahPos, megidolaonPos;

    bool hitAGI, hitMabufu, hitMaziodyne, hitSlash, hitMegidolaon;
    bool hitThisAttack;
    bool dashHasHit;

    int slashAnimTimer;
    int slashTotalFrames;

    int spaceChordBuffer;
    bool spaceWasDown;
    bool messiahChordConsumed;
    int animAccumulator;
    int personaAnimAccumulator;
    int noInputFrames;
    int introDisplayHold;
    int introLastFrame;
    int stanceEntryDelay;
    int actionVisualHold;
    int actionHoldState;
    int actionHoldFrame;

    bool meleeHitSparkActive;
    int meleeHitSparkFrame;
    D3DXVECTOR3 meleeHitSparkPos;

    void BeginMaziodyneSuper();
    void BeginMessiahSuper();

    void UpdateLiveEffectPositions(class Joker& enemy);
    void CompleteOneShotToStance(int lastFrame);
    void TickVisualHolds(int animSteps);

    bool CanUseSpaceChord() const;
    void TickSpaceChordBuffer(bool isJumpPressed, int steps);
    void BeginAirAttackState(int state, int frames);

    void UpdateInputAndMovement();
    void UpdateAnimation();
    void UpdatePersonaLogic(class Joker& enemy, int steps);
    void FinishPersonaSequence();
    void ApplyGravity(int steps);
    bool IsOnGround() const;
    void CheckAttackCollision(class Joker& enemy);
    void OnMeleeHitConnected(class Joker& enemy);
    void UpdateMeleeHitSpark(int steps);
    void RenderMeleeHitSpark(LPD3DXSPRITE sprite);
public:
    Makoto();
    ~Makoto();

    void Update() override;
    void Render(LPD3DXSPRITE sprite) override;
    void TakeDamage(int damage) override;
    void Reset() override;

    int GetCurrentState() const { return currentState; }
    int GetCurrentFrame() const { return currentFrame; }
    bool IsSuperMoveActive() const { return isSuperMoveActive; }
    bool IsGray() const { return isMakotoGray; }
    D3DCOLOR GetOverlayColor() const { return overlayColor; }

    int GetFacingDirection() const { return facingDirection; }
    const D3DXVECTOR3& GetPosition() const { return position; }

    bool IsAttacking() const;

    void RenderDebugHitbox(LPD3DXSPRITE sprite);
};

bool LoadMakotoTextures();
void CleanUpMakotoTextures();