#include "GameLogic.h"
#include "TutorialGuide.h"
#include "Collision.h"
#include "Physics.h"
#include "Input.h"
#include "player/makoto/Makoto.h"
#include "player/joker/Joker.h"
#include "player/narukami/Narukami.h"
#include "player/yosuke/Yosuke.h"
#include "UI.h"

static void ApplyBattleResultPoses();
bool IsBattleCombatActive();
bool IsBattleEndSequence();

static void ApplyTutorialPerksForFighter(Fighter& fighter, int steps);

struct TutorialRefillTracker {
    int delayFrames = 0;
    int lastHealth = -1;
    int lastSp = -1;
    float lastStamina = -1.0f;
};

static TutorialRefillTracker g_TutorialRefillP1;
static TutorialRefillTracker g_TutorialRefillP2;

static void ResetTutorialRefillTracker(TutorialRefillTracker& tracker, const Fighter* fighter) {
    tracker.delayFrames = 0;
    if (fighter) {
        tracker.lastHealth = fighter->GetHealth();
        tracker.lastSp = fighter->GetSp();
        tracker.lastStamina = fighter->GetStamina();
    }
    else {
        tracker.lastHealth = -1;
        tracker.lastSp = -1;
        tracker.lastStamina = -1.0f;
    }
}

static void ResetTutorialRefillTrackers() {
    ResetTutorialRefillTracker(g_TutorialRefillP1, g_Player1);
    ResetTutorialRefillTracker(g_TutorialRefillP2, g_Player2);
}

static void ProcessTutorialRefill(Fighter& fighter, TutorialRefillTracker& tracker, int steps) {
    const int health = fighter.GetHealth();
    const int maxHealth = fighter.GetMaxHealth();
    const int sp = fighter.GetSp();
    const int maxSp = fighter.GetMaxSp();
    const float stamina = fighter.GetStamina();
    const float maxStamina = fighter.GetMaxStamina();

    const bool depleted = health < maxHealth || sp < maxSp || stamina + 0.001f < maxStamina;
    if (!depleted) {
        ResetTutorialRefillTracker(tracker, &fighter);
        return;
    }

    const bool newDepletion =
        tracker.lastHealth < 0 ||
        health < tracker.lastHealth ||
        sp < tracker.lastSp ||
        stamina + 0.001f < tracker.lastStamina;

    if (newDepletion) {
        tracker.delayFrames = TUTORIAL_INFINITE_REFILL_DELAY_FRAMES;
    }

    tracker.lastHealth = health;
    tracker.lastSp = sp;
    tracker.lastStamina = stamina;

    tracker.delayFrames -= steps;
    if (tracker.delayFrames <= 0) {
        ApplyTutorialPerksForFighter(fighter, steps);
        ResetTutorialRefillTracker(tracker, &fighter);
    }
}

FrameTimer g_GameTimer;
Fighter* g_Player1 = nullptr;
Fighter* g_Player2 = nullptr;
bool g_ShowDebugHitboxes = false;
bool g_SuppressBattleDebugOverlay = false;

CharacterId g_SelectedP1 = Char_Makoto;
CharacterId g_SelectedP2 = Char_Joker;
BattleMode g_SelectedBattleMode = BattleMode::Battle;
static bool g_TutorialCpuAiEnabled = false;
static bool g_TutorialInfiniteHpSpEnabled = true;

bool IsTutorialCpuAiEnabled() {
    return g_TutorialCpuAiEnabled;
}

bool IsTutorialInfiniteHpSpEnabled() {
    return g_TutorialInfiniteHpSpEnabled;
}

void ToggleTutorialInfiniteHpSp() {
    if (!IsTutorialBattleMode()) return;
    g_TutorialInfiniteHpSpEnabled = !g_TutorialInfiniteHpSpEnabled;
    if (g_TutorialInfiniteHpSpEnabled) {
        RefillTutorialFighters();
    }
}

void ResetTutorialInfiniteHpSp() {
    g_TutorialInfiniteHpSpEnabled = true;
}

bool IsTutorialSandbagMode() {
    return IsTutorialBattleMode() && !g_TutorialCpuAiEnabled;
}

void ToggleTutorialCpuAi() {
    if (!IsTutorialBattleMode()) return;
    g_TutorialCpuAiEnabled = !g_TutorialCpuAiEnabled;
}

void ResetTutorialCpuAi() {
    g_TutorialCpuAiEnabled = false;
    ResetTutorialInfiniteHpSp();
}

bool IsTutorialBattleMode() {
    return g_SelectedBattleMode == BattleMode::Tutorial;
}

bool ShouldFighterDieOnZeroHealth() {
    return !IsTutorialBattleMode();
}

static Fighter* g_DamageAttacker = nullptr;
static int g_P1HitCombo = 0;
static int g_HitComboTimeoutSteps = 0;

int GetP1HitCombo() {
    return g_P1HitCombo;
}

void DealMeleeHit(Fighter& attacker, Fighter& defender, int damage) {
    g_DamageAttacker = &attacker;
    LogCollisionDetected("fighter attack hit");
    defender.TakeDamage(damage);
    g_DamageAttacker = nullptr;
}

void DealSkillHit(Fighter& attacker, Fighter& defender, int damage) {
    g_DamageAttacker = &attacker;
    LogCollisionDetected("fighter skill hit");
    defender.ApplySkillDamage(damage);
    g_DamageAttacker = nullptr;
}

static void TryTriggerHitComboWin() {
    if (IsTutorialBattleMode() || !g_Player2 || IsBattleEndSequence()) return;
    if (g_Player2->GetHealth() > HIT_COMBO_TARGET_HP) return;

    g_BattlePlayer1Won = true;
    g_BattleFlowPhase = BattleFlowPhase::Knockout;
    g_BattleFlowTimer = 0;
    ApplyBattleResultPoses();
}

void NotifyFighterDamageApplied(Fighter& victim, int appliedDamage) {
    if (appliedDamage <= 0) return;

    if (IsTutorialBattleMode()) {
        return;
    }

    if (!IsBattleCombatActive()) return;
    if (!g_DamageAttacker || g_DamageAttacker->GetPlayerSlot() == victim.GetPlayerSlot()) return;

    if (g_DamageAttacker->IsHumanControlled() && !victim.IsHumanControlled()) {
        g_P1HitCombo++;
        g_HitComboTimeoutSteps = HIT_COMBO_TIMEOUT_STEPS;
    }

    TryTriggerHitComboWin();
}

void BeginBattleLogicFrame() {
    // Lecture 5: QPC frame timer catches up on slow PCs; fast PCs stay at 1 step/frame.
    if (g_GameTimer.FramesToUpdate() <= 0) {
        g_GameTimer.SetLogicSteps(BATTLE_LOGIC_STEPS_PER_FRAME);
    }
}

static void ApplyTutorialPerksForFighter(Fighter& fighter, int steps) {
    (void)steps;

    fighter.health = fighter.maxHealth;
    fighter.isDead = false;
    fighter.sp = fighter.maxSp;
    fighter.RefillStamina();
}

void RefillTutorialFighters() {
    if (!IsTutorialBattleMode() || !IsTutorialInfiniteHpSpEnabled()) return;
    if (g_Player1) ApplyTutorialPerksForFighter(*g_Player1, 1);
    if (g_Player2) ApplyTutorialPerksForFighter(*g_Player2, 1);
    ResetTutorialRefillTrackers();
}

void ApplyTutorialModePerks(int steps) {
    if (!IsTutorialBattleMode() || IsBattleEndSequence()) return;
    if (!IsTutorialInfiniteHpSpEnabled()) return;
    if (steps <= 0) steps = BATTLE_LOGIC_STEPS_PER_FRAME;

    if (g_Player1) ProcessTutorialRefill(*g_Player1, g_TutorialRefillP1, steps);
    if (g_Player2) ProcessTutorialRefill(*g_Player2, g_TutorialRefillP2, steps);
}

void PositionFightersAtDefaultSpawn() {
    if (!g_Player1 || !g_Player2) return;

    g_Player1->ApplySlotSpawnDefaults();
    g_Player2->ApplySlotSpawnDefaults();
    g_Player1->UpdateScaledHurtbox();
    g_Player2->UpdateScaledHurtbox();
}

BattleFlowPhase g_BattleFlowPhase = BattleFlowPhase::Countdown;
int g_BattleFlowTimer = 0;
int g_BattleCountdownDigit = 3;
int g_BattleTimeRemainingSteps = BATTLE_ROUND_TIME_STEPS;
bool g_BattlePlayer1Won = true;

Fighter* CreateFighter(CharacterId id, int slot, bool humanControlled) {
    Fighter* fighter = nullptr;
    switch (id) {
    case Char_Makoto:
        fighter = new Makoto();
        break;
    case Char_Joker:
        fighter = new Joker();
        break;
    case Char_Narukami:
        fighter = new Narukami();
        break;
    case Char_Yosuke:
        fighter = new Yosuke();
        break;
    default:
        fighter = new Makoto();
        id = Char_Makoto;
        break;
    }

    fighter->SetCharacterId(id);
    fighter->SetPlayerSlot(slot);
    fighter->SetHumanControlled(humanControlled);
    fighter->Reset();
    fighter->ResetAiState();
    return fighter;
}

void DestroyFighters() {
    delete g_Player1;
    delete g_Player2;
    g_Player1 = nullptr;
    g_Player2 = nullptr;
}

void SetupBattleFighters(CharacterId p1Id, CharacterId p2Id) {
    DestroyFighters();
    g_SelectedP1 = p1Id;
    g_SelectedP2 = p2Id;
    g_Player1 = CreateFighter(p1Id, 0, true);
    g_Player2 = CreateFighter(p2Id, 1, false);
    ResetTutorialCpuAi();
}

Fighter* GetOpponent(const Fighter& self) {
    if (&self == g_Player1) return g_Player2;
    if (&self == g_Player2) return g_Player1;
    return g_Player2;
}

void PullEnemyForUltimate(Fighter& attacker, Fighter& enemy, bool pullToGround) {
    if (enemy.IsDead()) return;

    const float gap = GetDefaultBattleCenterGap();
    const float targetX =
        attacker.position.x + (float)attacker.GetFacingDirection() * gap;
    enemy.position.x += (targetX - enemy.position.x) * ULTIMATE_PULL_LERP;

    const AABB atkBox = attacker.GetBodyCollisionBox();
    const float enemyHalf = enemy.GetBodyCollisionBox().width * 0.5f;
    if (attacker.GetFacingDirection() > 0) {
        const float minCenterX = atkBox.x + atkBox.width + enemyHalf + BODY_COLLISION_EPSILON;
        if (enemy.position.x < minCenterX) {
            enemy.position.x = minCenterX;
        }
    }
    else {
        const float maxCenterX = atkBox.x - enemyHalf - BODY_COLLISION_EPSILON;
        if (enemy.position.x > maxCenterX) {
            enemy.position.x = maxCenterX;
        }
    }

    const float minX = ULTIMATE_PULL_SCREEN_MARGIN;
    const float maxX = (float)SCREEN_WIDTH - ULTIMATE_PULL_SCREEN_MARGIN;
    if (enemy.position.x < minX) enemy.position.x = minX;
    if (enemy.position.x > maxX) enemy.position.x = maxX;

    if (pullToGround) {
        enemy.position.y += (CHARACTER_GROUND_Y - enemy.position.y) * ULTIMATE_PULL_LERP;
        if (fabsf(enemy.position.y - CHARACTER_GROUND_Y) < GROUND_CONTACT_EPSILON) {
            enemy.position.y = CHARACTER_GROUND_Y;
        }
    }

    enemy.UpdateScaledHurtbox();
    ClampFighterAgainstOpponent(enemy, attacker);
}

void SnapFighterStandForUltimate(Fighter& fighter) {
    fighter.isHit = false;
    fighter.hitStunTimer = 0;
    fighter.position.y = CHARACTER_GROUND_Y;

    switch (fighter.GetCharacterId()) {
    case Char_Joker:
        static_cast<Joker&>(fighter).SnapStandForUltimate();
        break;
    case Char_Makoto:
        static_cast<Makoto&>(fighter).SnapStandForUltimate();
        break;
    case Char_Narukami:
        static_cast<Narukami&>(fighter).SnapStandForUltimate();
        break;
    case Char_Yosuke:
        static_cast<Yosuke&>(fighter).SnapStandForUltimate();
        break;
    default:
        break;
    }

    fighter.UpdateScaledHurtbox();
}

static void ClampFighterPushX(Fighter& fighter) {
    const AABB box = fighter.GetBodyCollisionBox();
    ClampFighterCenterX(fighter.position.x, box.width * 0.5f);
}

static float GetLiveMinCenterGap(const Fighter& a, const Fighter& b) {
    return a.GetBodyCollisionBox().width * 0.5f +
        b.GetBodyCollisionBox().width * 0.5f +
        BODY_COLLISION_EPSILON;
}

static bool ArePushboxesSeparated(const Fighter& left, const Fighter& right) {
    const AABB leftBox = left.GetBodyCollisionBox();
    const AABB rightBox = right.GetBodyCollisionBox();
    return rightBox.x >= leftBox.x + leftBox.width + BODY_COLLISION_EPSILON;
}

void ClampFighterAgainstOpponent(Fighter& self, Fighter& opponent) {
    if (self.GetPlayerSlot() < opponent.GetPlayerSlot()) {
        const AABB oppBox = opponent.GetBodyCollisionBox();
        AABB selfBox = self.GetBodyCollisionBox();
        const float maxRight = oppBox.x - BODY_COLLISION_EPSILON;
        const float overflow = (selfBox.x + selfBox.width) - maxRight;
        if (overflow > 0.0f) {
            self.position.x -= overflow;
        }
    }
    else if (self.GetPlayerSlot() > opponent.GetPlayerSlot()) {
        const AABB oppBox = opponent.GetBodyCollisionBox();
        AABB selfBox = self.GetBodyCollisionBox();
        const float minLeft = oppBox.x + oppBox.width + BODY_COLLISION_EPSILON;
        const float overflow = minLeft - selfBox.x;
        if (overflow > 0.0f) {
            self.position.x += overflow;
        }
    }

    ClampFighterPushX(self);
    self.UpdateScaledHurtbox();
}

float GetFighterHorizontalGap(const Fighter& a, const Fighter& b) {
    const AABB boxA = a.GetBodyCollisionBox();
    const AABB boxB = b.GetBodyCollisionBox();
    const float aRight = boxA.x + boxA.width;
    const float bRight = boxB.x + boxB.width;

    if (aRight <= boxB.x) {
        return boxB.x - aRight;
    }
    if (bRight <= boxA.x) {
        return boxA.x - bRight;
    }
    return 0.0f;
}

static AABB BuildMeleeAttackAabb(
    const D3DXVECTOR3& fighterPos,
    int facingDirection,
    float scale,
    const AttackData& data)
{
    AABB box;
    box.width = data.width * scale;
    box.height = data.height * scale;
    const float forward = data.offsetX * scale;
    const float vertical = data.offsetY * scale;

    if (facingDirection == 1) {
        box.x = fighterPos.x + forward;
    }
    else {
        box.x = fighterPos.x - forward - box.width;
    }
    box.y = fighterPos.y + vertical;
    return box;
}

bool WouldMeleeAttackReach(const Fighter& attacker, const Fighter& defender, const AttackData& data) {
    const AABB attackBox = BuildMeleeAttackAabb(
        attacker.GetPosition(),
        attacker.GetFacingDirection(),
        GetCharacterRenderScale(),
        data);
    return GetAabbHorizontalGap(attackBox, defender.GetHurtbox()) <= 0.0f;
}

bool IsFighterInMeleeStrikeRange(const Fighter& attacker, const Fighter& defender) {
    const AttackData neutral = {
        ATK_NEUTRAL_START, ATK_NEUTRAL_END, ATK_NEUTRAL_DAMAGE,
        ATK_NEUTRAL_OFFSET_X, ATK_NEUTRAL_OFFSET_Y, ATK_NEUTRAL_WIDTH, ATK_NEUTRAL_HEIGHT
    };
    return WouldMeleeAttackReach(attacker, defender, neutral);
}

static void EnforceFighterSideOrder(Fighter& a, Fighter& b) {
    Fighter* p1 = (a.GetPlayerSlot() == 0) ? &a : &b;
    Fighter* p2 = (a.GetPlayerSlot() == 1) ? &a : &b;

    ClampFighterAgainstOpponent(*p1, *p2);
    ClampFighterAgainstOpponent(*p2, *p1);

    if (ArePushboxesSeparated(*p1, *p2)) {
        return;
    }

    if (IsTutorialBattleMode() && p1->IsHumanControlled() && !p2->IsHumanControlled()) {
        const AABB oppBox = p2->GetBodyCollisionBox();
        const AABB selfBox = p1->GetBodyCollisionBox();
        p1->position.x -= (selfBox.x + selfBox.width) - (oppBox.x - BODY_COLLISION_EPSILON);
        ClampFighterPushX(*p1);
        p1->UpdateScaledHurtbox();
        return;
    }
    if (IsTutorialBattleMode() && !p1->IsHumanControlled() && p2->IsHumanControlled()) {
        const AABB oppBox = p1->GetBodyCollisionBox();
        const AABB selfBox = p2->GetBodyCollisionBox();
        p2->position.x += (oppBox.x + oppBox.width + BODY_COLLISION_EPSILON) - selfBox.x;
        ClampFighterPushX(*p2);
        p2->UpdateScaledHurtbox();
        return;
    }

    const AABB boxP1 = p1->GetBodyCollisionBox();
    const AABB boxP2 = p2->GetBodyCollisionBox();
    const float overlap =
        std::fmin(boxP1.x + boxP1.width, boxP2.x + boxP2.width) - std::fmax(boxP1.x, boxP2.x);
    if (overlap > 0.0f) {
        p1->position.x -= overlap * 0.5f;
        p2->position.x += overlap * 0.5f;
        ClampFighterPushX(*p1);
        ClampFighterPushX(*p2);
        p1->UpdateScaledHurtbox();
        p2->UpdateScaledHurtbox();
    }

    ClampFighterAgainstOpponent(*p1, *p2);
    ClampFighterAgainstOpponent(*p2, *p1);
}

// Body push (BMCS2224 Physics / collision response).
// Side-view fighters separate on X only. Uses AABB detection from CollisionHelper /
// PhysicsWorld; sandbag stays put while the human is pushed out.
void ResolveFighterBodyOverlap(Fighter& a, Fighter& b) {
    const AABB boxA = a.GetBodyCollisionBox();
    const AABB boxB = b.GetBodyCollisionBox();
    static bool s_LoggedBodyOverlap = false;
    if (CollisionHelper::AABBIntersect(boxA, boxB)) {
        if (!s_LoggedBodyOverlap) {
            LogCollisionDetected("fighter body overlap");
            s_LoggedBodyOverlap = true;
        }
        const float overlapLeft = (boxA.x + boxA.width) - boxB.x;
        const float overlapRight = (boxB.x + boxB.width) - boxA.x;
        if (overlapLeft > 0.0f || overlapRight > 0.0f) {
            float deltaA = 0.0f;
            float deltaB = 0.0f;
            if (overlapLeft > 0.0f && overlapLeft <= overlapRight) {
                deltaA = -overlapLeft;
                deltaB = overlapLeft;
            }
            else if (overlapRight > 0.0f) {
                deltaA = overlapRight;
                deltaB = -overlapRight;
            }

            // Also exercise PhysicsWorld overlap resolver (horizontal component).
            float resolvedPushX = 0.0f;
            float resolvedPushY = 0.0f;
            AABB movingProbe = boxA;
            if (PhysicsWorld::ResolveOverlap(movingProbe, boxB, resolvedPushX, resolvedPushY) &&
                fabsf(resolvedPushX) > fabsf(resolvedPushY) &&
                fabsf(resolvedPushX) > 0.001f) {
                deltaA = resolvedPushX;
                deltaB = -resolvedPushX;
            }

            const bool aHuman = a.IsHumanControlled();
            const bool bHuman = b.IsHumanControlled();

            if (!aHuman && bHuman) {
                b.position.x += deltaB;
                ClampFighterPushX(b);
            }
            else if (aHuman && !bHuman) {
                a.position.x += deltaA;
                ClampFighterPushX(a);
                if (!IsTutorialBattleMode()) {
                    b.position.x += deltaB;
                    ClampFighterPushX(b);
                }
            }
            else {
                a.position.x += deltaA * 0.5f;
                b.position.x += deltaB * 0.5f;
                ClampFighterPushX(a);
                ClampFighterPushX(b);
            }

            a.UpdateScaledHurtbox();
            b.UpdateScaledHurtbox();
        }
    }
    else {
        s_LoggedBodyOverlap = false;
    }

    EnforceFighterSideOrder(a, b);
}

void EnforceFighterGroundSeparation(Fighter& a, Fighter& b) {
    ClampFighterAgainstOpponent(a, b);
    ClampFighterAgainstOpponent(b, a);
    EnforceFighterSideOrder(a, b);
    a.UpdateScaledHurtbox();
    b.UpdateScaledHurtbox();
}

static bool g_BattleExitHandled = false;
static bool g_BattleResultPosesApplied = false;

static void ApplyBattleResultPoses() {
    if (g_BattleResultPosesApplied || !g_Player1 || !g_Player2) return;
    g_BattleResultPosesApplied = true;

    if (g_BattlePlayer1Won) {
        g_Player1->BeginVictoryPose();
        g_Player2->BeginDefeatPose();
    }
    else {
        g_Player2->BeginVictoryPose();
        g_Player1->BeginDefeatPose();
    }
}

void EnsureBattleResultPosesApplied() {
    if (IsTutorialBattleMode()) return;
    if (g_BattleResultPosesApplied || !g_Player1 || !g_Player2) return;

    const bool p1Dead = g_Player1->IsDead();
    const bool p2Dead = g_Player2->IsDead();
    if (!p1Dead && !p2Dead) return;

    g_BattlePlayer1Won = p2Dead && !p1Dead;
    if (p1Dead && p2Dead) {
        g_BattlePlayer1Won = false;
    }

    if (g_BattleFlowPhase == BattleFlowPhase::Fight) {
        g_BattleFlowPhase = BattleFlowPhase::Knockout;
        g_BattleFlowTimer = 0;
    }

    ApplyBattleResultPoses();
}

void ResetBattleFlow() {
    g_BattleExitHandled = false;
    g_BattleResultPosesApplied = false;

    if (IsTutorialBattleMode()) {
        g_BattleFlowPhase = BattleFlowPhase::Fight;
        g_BattleFlowTimer = 0;
        g_BattleCountdownDigit = 0;
        g_BattleTimeRemainingSteps = 0;
    }
    else {
        g_BattleFlowPhase = BattleFlowPhase::Countdown;
        g_BattleFlowTimer = 0;
        g_BattleCountdownDigit = 3;
        g_BattleTimeRemainingSteps = BATTLE_ROUND_TIME_STEPS;
    }
    g_P1HitCombo = 0;
    g_HitComboTimeoutSteps = 0;
    g_BattlePlayer1Won = true;
    if (IsTutorialBattleMode()) {
        ResetTutorialGuide();
        PositionFightersAtDefaultSpawn();
    }
}

bool IsBattleCombatActive() {
    return g_BattleFlowPhase == BattleFlowPhase::Fight;
}

bool IsBattleInputAllowed() {
    return g_BattleFlowPhase == BattleFlowPhase::Fight;
}

bool ShouldShowBattleKo() {
    return g_BattleFlowPhase == BattleFlowPhase::Knockout;
}

bool IsBattleEndSequence() {
    return g_BattleFlowPhase == BattleFlowPhase::Knockout ||
        g_BattleFlowPhase == BattleFlowPhase::ResultPose ||
        g_BattleFlowPhase == BattleFlowPhase::FadeOut ||
        g_BattleFlowPhase == BattleFlowPhase::Finished;
}

const char* GetBattleCountdownLabel() {
    if (g_BattleFlowPhase != BattleFlowPhase::Countdown) return nullptr;
    if (g_BattleCountdownDigit >= 1) {
        static char digitBuf[2] = {};
        digitBuf[0] = (char)('0' + g_BattleCountdownDigit);
        digitBuf[1] = '\0';
        return digitBuf;
    }
    return "FIGHT";
}

bool IsHumanPlayerEngaged() {
    // Read real keyboard/mouse (not AI overlay) so P2 AI can mirror "player idle".
    if (g_WindowHasFocus) {
        if (g_InputManager.IsKeyDown(DIK_A) || g_InputManager.IsKeyDown(DIK_D) ||
            g_InputManager.IsKeyDown(DIK_LEFT) || g_InputManager.IsKeyDown(DIK_RIGHT) ||
            g_InputManager.IsKeyDown(DIK_SPACE) || g_InputManager.IsKeyDown(DIK_LSHIFT) ||
            g_InputManager.IsKeyDown(DIK_RSHIFT) || g_InputManager.IsKeyDown(DIK_E) ||
            g_InputManager.IsKeyDown(DIK_R) || g_InputManager.IsKeyDown(DIK_S) ||
            g_InputManager.IsKeyDown(DIK_C) || g_InputManager.IsKeyDown(DIK_J) ||
            g_InputManager.IsKeyDown(DIK_I) || g_InputManager.IsKeyDown(DIK_T) ||
            g_InputManager.IsKeyDown(DIK_1) || g_InputManager.IsKeyDown(DIK_2) ||
            g_InputManager.IsKeyDown(DIK_3) || g_InputManager.IsKeyDown(DIK_4) ||
            g_InputManager.IsKeyDown(DIK_5)) {
            return true;
        }
        if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0 ||
            (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0) {
            return true;
        }
    }
    if (g_Player1 && g_Player1->IsInCombatAction()) return true;
    return false;
}

int GetGuardAwayDirection(const Fighter& self) {
    if (Fighter* opponent = GetOpponent(self)) {
        const float dx = opponent->GetPosition().x - self.GetPosition().x;
        if (dx > 1.0f) return -1;
        if (dx < -1.0f) return 1;
    }
    return -self.GetFacingDirection();
}

int GetGuardTowardDirection(const Fighter& self) {
    return -GetGuardAwayDirection(self);
}

HorizontalMoveInput ReadHorizontalMoveInput() {
    HorizontalMoveInput out;
    out.leftHeld = IsGameKeyDown(DIK_LEFT) || IsGameKeyDown(DIK_A);
    out.rightHeld = IsGameKeyDown(DIK_RIGHT) || IsGameKeyDown(DIK_D);
    if (out.leftHeld && !out.rightHeld) {
        out.isMoving = true;
        out.moveDirX = -1.0f;
    }
    else if (out.rightHeld && !out.leftHeld) {
        out.isMoving = true;
        out.moveDirX = 1.0f;
    }
    return out;
}

bool IsHoldingBackInput(const Fighter& self) {
    const int away = GetGuardAwayDirection(self);
    if (away < 0) {
        return IsGameKeyDown(DIK_LEFT) || IsGameKeyDown(DIK_A);
    }
    return IsGameKeyDown(DIK_RIGHT) || IsGameKeyDown(DIK_D);
}

// Guard: hold back (away from opponent) + S. Shift is reserved for Run.
bool IsHoldingGuardInput(const Fighter& self) {
    if (IsGameKeyDown(DIK_LSHIFT) || IsGameKeyDown(DIK_RSHIFT)) {
        return false;
    }
    return IsHoldingBackInput(self) && IsGameKeyDown(DIK_S);
}

void ApplyGuardFacingTowardOpponent(Fighter& self) {
    if (IsHoldingGuardInput(self) || self.IsInGuardState()) {
        self.facingDirection = GetGuardTowardDirection(self);
    }
}

float ComputeSmoothedLocomotionSpeed(int baseVelocity, bool wantsRun, float& runBlend, int steps) {
    const float target = wantsRun ? 1.0f : 0.0f;
    const float delta = FIGHTER_RUN_BLEND_RATE * (float)steps;
    if (runBlend < target) {
        runBlend += delta;
        if (runBlend > target) runBlend = target;
    }
    else if (runBlend > target) {
        runBlend -= delta;
        if (runBlend < target) runBlend = target;
    }
    const float multiplier = 1.0f + runBlend;
    return (float)baseVelocity * multiplier;
}

bool ShouldUseRunLocomotion(float runBlend) {
    return runBlend >= FIGHTER_RUN_ANIM_THRESHOLD;
}

bool IsFighterAirborne(const Fighter& self) {
    return self.GetPosition().y < CHARACTER_GROUND_Y - GROUND_CONTACT_EPSILON;
}

bool TryProcessGuardBlock(Fighter& defender, int rawDamage, int& appliedDamage) {
    appliedDamage = rawDamage;
    if (rawDamage <= 0 || defender.IsDead() || defender.IsPlayingResultPose()) {
        return false;
    }
    if (!IsHoldingGuardInput(defender)) {
        return false;
    }
    if (!defender.IsInGuardState() && defender.IsInCombatAction()) {
        return false;
    }

    defender.HoldGuardState(IsFighterAirborne(defender));
    if (!defender.IsInGuardState() || !IsHoldingGuardInput(defender)) {
        return false;
    }

    appliedDamage = GUARD_CHIP_DAMAGE;
    return true;
}

void UpdateBattleFlow() {
    int steps = g_GameTimer.GetLastFramesToUpdate();
    if (steps <= 0) steps = BATTLE_LOGIC_STEPS_PER_FRAME;

    switch (g_BattleFlowPhase) {
    case BattleFlowPhase::Countdown:
        g_BattleFlowTimer += steps;
        if (g_BattleCountdownDigit >= 1) {
            if (g_BattleFlowTimer >= BATTLE_COUNTDOWN_DIGIT_STEPS) {
                g_BattleFlowTimer = 0;
                --g_BattleCountdownDigit;
            }
        }
        else if (g_BattleFlowTimer >= BATTLE_COUNTDOWN_FIGHT_STEPS) {
            g_BattleFlowPhase = BattleFlowPhase::Fight;
            g_BattleFlowTimer = 0;
        }
        break;

    case BattleFlowPhase::Fight:
        if (!IsTutorialBattleMode()) {
            if (g_HitComboTimeoutSteps > 0) {
                g_HitComboTimeoutSteps -= steps;
                if (g_HitComboTimeoutSteps <= 0) {
                    g_HitComboTimeoutSteps = 0;
                    g_P1HitCombo = 0;
                }
            }

            g_BattleTimeRemainingSteps -= steps;
            if (g_BattleTimeRemainingSteps < 0) {
                g_BattleTimeRemainingSteps = 0;
            }
            if (g_BattleTimeRemainingSteps == 0 && g_Player1 && g_Player2) {
                const int p2Hp = g_Player2->GetHealth();
                g_BattlePlayer1Won = (p2Hp <= HIT_COMBO_TARGET_HP);
                g_BattleFlowPhase = BattleFlowPhase::Knockout;
                g_BattleFlowTimer = 0;
                ApplyBattleResultPoses();
                break;
            }
        }

        if (!IsTutorialBattleMode() &&
            g_Player1 && g_Player2 &&
            (g_Player1->IsDead() || g_Player2->IsDead())) {
            g_BattlePlayer1Won = g_Player2->IsDead() && !g_Player1->IsDead();
            if (g_Player1->IsDead() && g_Player2->IsDead()) {
                g_BattlePlayer1Won = false;
            }
            g_BattleFlowPhase = BattleFlowPhase::Knockout;
            g_BattleFlowTimer = 0;
            ApplyBattleResultPoses();
        }
        break;

    case BattleFlowPhase::Knockout:
        g_BattleFlowTimer += steps;
        if (g_BattleFlowTimer >= BATTLE_KO_HOLD_STEPS) {
            g_BattleFlowPhase = BattleFlowPhase::ResultPose;
            g_BattleFlowTimer = 0;
        }
        break;

    case BattleFlowPhase::ResultPose:
        g_BattleFlowTimer += steps;
        if (g_BattleFlowTimer >= BATTLE_RESULT_POSE_MIN_STEPS) {
            g_BattleFlowPhase = BattleFlowPhase::FadeOut;
            g_BattleFlowTimer = 0;
        }
        break;

    case BattleFlowPhase::FadeOut:
        g_BattleFlowTimer += steps;
        if (g_BattleFlowTimer >= BATTLE_FADE_OUT_STEPS) {
            g_BattleFlowPhase = BattleFlowPhase::Finished;
            g_BattleFlowTimer = 0;
        }
        break;

    case BattleFlowPhase::Finished:
        g_BattleFlowTimer += steps;
        break;
    }
}

bool ConsumeBattleFinishedExit() {
    if (g_BattleFlowPhase != BattleFlowPhase::Finished || g_BattleExitHandled) {
        return false;
    }
    if (g_BattleFlowTimer < BATTLE_FINISHED_BLACK_HOLD_STEPS) {
        return false;
    }
    g_BattleExitHandled = true;
    return true;
}

void ApplyBattleRenderTints() {
    if (!g_Player1 || !g_Player2) return;

    const D3DCOLOR white = D3DCOLOR_XRGB(255, 255, 255);
    g_Player1->SetSpriteTint(white);
    g_Player2->SetSpriteTint(white);

    if (g_SelectedP1 != g_SelectedP2) return;

    // Same character: P2 stays gold-yellow for the whole fight (body + persona + FX).
    const D3DCOLOR mirrorP2Yellow = D3DCOLOR_XRGB(
        MIRROR_MATCH_P2_TINT_R,
        MIRROR_MATCH_P2_TINT_G,
        MIRROR_MATCH_P2_TINT_B);
    g_Player2->SetSpriteTint(mirrorP2Yellow);
}
