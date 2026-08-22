#pragma once
#include "config.h"
#include "FrameTimer.h"
#include "player/Fighter.h"
#include "player/CharacterId.h"

// =============================================================================
// Game logic module (BMCS2224) — OO fighter factory + shared battle services.
// CreateFighter() builds Makoto/Joker/Narukami behind Fighter*; body overlap
// resolve keeps all characters from walking through each other.
// =============================================================================

extern FrameTimer g_GameTimer;
extern Fighter* g_Player1;
extern Fighter* g_Player2;
extern bool g_ShowDebugHitboxes;

extern CharacterId g_SelectedP1;
extern CharacterId g_SelectedP2;

enum class BattleMode {
    Battle,
    Tutorial
};

extern BattleMode g_SelectedBattleMode;
bool IsTutorialBattleMode();
void BeginBattleLogicFrame();
void ApplyTutorialModePerks(int steps);

Fighter* CreateFighter(CharacterId id, int slot, bool humanControlled);
void DestroyFighters();
void SetupBattleFighters(CharacterId p1Id, CharacterId p2Id);

// Opposite fighter for collision / persona targeting.
Fighter* GetOpponent(const Fighter& self);

// Ultimate cinematic: forcibly pull the foe in front of the attacker (X + optional ground).
void PullEnemyForUltimate(Fighter& attacker, Fighter& enemy, bool pullToGround = true);

// Body push: keep fighters from walking through each other (all characters).
void ResolveFighterBodyOverlap(Fighter& a, Fighter& b);

// Slot-aware clamp: P1 stays left of P2 (Makoto-style solid body).
void ClampFighterAgainstOpponent(Fighter& self, Fighter& opponent);

// Hard separation after both fighters move — fixes dash cross-through.
void EnforceFighterGroundSeparation(Fighter& a, Fighter& b);

// Round flow: 3-2-1 -> fight -> KO -> win/lose poses -> result screen.
enum class BattleFlowPhase {
    Countdown,
    Fight,
    Knockout,
    ResultPose,
    FadeOut,
    Finished
};

extern BattleFlowPhase g_BattleFlowPhase;
extern int g_BattleFlowTimer;
extern int g_BattleCountdownDigit;
extern int g_BattleTimeRemainingSteps;
extern bool g_BattlePlayer1Won;

void ResetBattleFlow();
void UpdateBattleFlow();
void EnsureBattleResultPosesApplied();
bool ConsumeBattleFinishedExit();
bool IsBattleCombatActive();
bool IsBattleInputAllowed();
const char* GetBattleCountdownLabel();
bool ShouldShowBattleKo();
bool IsBattleEndSequence();
bool IsHumanPlayerEngaged();
void ApplyBattleRenderTints();

// Guard: hold direction away from the opponent (not toward them).
int GetGuardAwayDirection(const Fighter& self);
int GetGuardTowardDirection(const Fighter& self);
bool IsHoldingGuardInput(const Fighter& self);
bool IsFighterAirborne(const Fighter& self);
bool TryProcessGuardBlock(Fighter& defender, int rawDamage, int& appliedDamage);
