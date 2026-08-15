#include "game_logic.h"
#include "collision.h"
#include "physics.h"
#include "input.h"
#include "player/makoto/Makoto.h"
#include "player/joker/Joker.h"
#include "player/narukami/Narukami.h"
#include "ui.h"

FrameTimer g_GameTimer;
Fighter* g_Player1 = nullptr;
Fighter* g_Player2 = nullptr;
bool g_ShowDebugHitboxes = false;

CharacterId g_SelectedP1 = Char_Makoto;
CharacterId g_SelectedP2 = Char_Joker;
BattleMode g_SelectedBattleMode = BattleMode::Battle;

bool IsTutorialBattleMode() {
    return g_SelectedBattleMode == BattleMode::Tutorial;
}

static int g_TutorialRecoverIdleFrames[2] = { 0, 0 };

static void ApplyTutorialPerksForFighter(Fighter& fighter, int steps) {
    const int slot = fighter.GetPlayerSlot();
    if (slot < 0 || slot > 1) return;

    if (fighter.IsHumanControlled()) {
        fighter.sp = fighter.maxSp;
        fighter.RestoreStamina(STAMINA_REGEN_PER_STEP * (float)steps * TUTORIAL_STAMINA_REGEN_MULTIPLIER);
    }

    if (fighter.health >= fighter.maxHealth || fighter.isHit || fighter.IsInCombatAction()) {
        g_TutorialRecoverIdleFrames[slot] = 0;
        return;
    }

    g_TutorialRecoverIdleFrames[slot] += steps;
    if (g_TutorialRecoverIdleFrames[slot] < TRAINING_HEAL_IDLE_FRAMES) return;

    fighter.health = fighter.maxHealth;
    fighter.isDead = false;
    SyncBattleHudHealth(slot + 1, fighter.health, fighter.maxHealth);
    if (fighter.IsHumanControlled()) {
        fighter.RefillStamina();
    }
    g_TutorialRecoverIdleFrames[slot] = 0;
}

void ApplyTutorialModePerks(int steps) {
    if (!IsTutorialBattleMode()) return;
    if (steps <= 0) steps = 1;
    if (steps > GAME_TIMER_MAX_STEPS_PER_FRAME) steps = GAME_TIMER_MAX_STEPS_PER_FRAME;

    if (g_Player1) ApplyTutorialPerksForFighter(*g_Player1, steps);
    if (g_Player2) ApplyTutorialPerksForFighter(*g_Player2, steps);
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
    default:
        fighter = new Makoto();
        id = Char_Makoto;
        break;
    }

    fighter->SetCharacterId(id);
    fighter->SetPlayerSlot(slot);
    fighter->SetHumanControlled(humanControlled);
    fighter->Reset();
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

static void ClampFighterPushX(Fighter& fighter) {
    const AABB box = fighter.GetBodyCollisionBox();
    ClampFighterCenterX(fighter.position.x, box.width * 0.5f);
}

static float GetLiveMinCenterGap(const Fighter& a, const Fighter& b) {
    return a.GetBodyCollisionBox().width * 0.5f +
        b.GetBodyCollisionBox().width * 0.5f +
        BODY_COLLISION_EPSILON;
}

void ClampFighterAgainstOpponent(Fighter& self, Fighter& opponent) {
    const float minGap = GetLiveMinCenterGap(self, opponent);

    // Side-view rule: slot 0 (P1) always left of slot 1 (P2). Never use visual side.
    if (self.GetPlayerSlot() < opponent.GetPlayerSlot()) {
        const float maxX = opponent.position.x - minGap;
        if (self.position.x > maxX) {
            self.position.x = maxX;
        }
    }
    else if (self.GetPlayerSlot() > opponent.GetPlayerSlot()) {
        const float minX = opponent.position.x + minGap;
        if (self.position.x < minX) {
            self.position.x = minX;
        }
    }

    ClampFighterPushX(self);
    self.UpdateScaledHurtbox();
}

static void EnforceFighterSideOrder(Fighter& a, Fighter& b) {
    Fighter* p1 = (a.GetPlayerSlot() == 0) ? &a : &b;
    Fighter* p2 = (a.GetPlayerSlot() == 1) ? &a : &b;
    const float minGap = GetLiveMinCenterGap(*p1, *p2);

    if (p2->position.x - p1->position.x >= minGap) {
        return;
    }

    if (IsTutorialBattleMode() && p1->IsHumanControlled() && !p2->IsHumanControlled()) {
        p1->position.x = p2->position.x - minGap;
        ClampFighterPushX(*p1);
        p1->UpdateScaledHurtbox();
        return;
    }
    if (IsTutorialBattleMode() && !p1->IsHumanControlled() && p2->IsHumanControlled()) {
        p2->position.x = p1->position.x + minGap;
        ClampFighterPushX(*p2);
        p2->UpdateScaledHurtbox();
        return;
    }

    const float mid = (p1->position.x + p2->position.x) * 0.5f;
    p1->position.x = mid - minGap * 0.5f;
    p2->position.x = mid + minGap * 0.5f;
    ClampFighterPushX(*p1);
    ClampFighterPushX(*p2);
    p1->UpdateScaledHurtbox();
    p2->UpdateScaledHurtbox();
}

// Body push (BMCS2224 Physics / collision response).
// Side-view fighters separate on X only. Uses AABB detection from CollisionHelper /
// PhysicsWorld; sandbag stays put while the human is pushed out.
void ResolveFighterBodyOverlap(Fighter& a, Fighter& b) {
    const AABB boxA = a.GetBodyCollisionBox();
    const AABB boxB = b.GetBodyCollisionBox();
    if (CollisionHelper::AABBIntersect(boxA, boxB)) {
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

    EnforceFighterSideOrder(a, b);
}

void EnforceFighterGroundSeparation(Fighter& a, Fighter& b) {
    ClampFighterAgainstOpponent(a, b);
    ClampFighterAgainstOpponent(b, a);
    EnforceFighterSideOrder(a, b);
    a.UpdateScaledHurtbox();
    b.UpdateScaledHurtbox();
}

void ResetBattleFlow() {
    g_TutorialRecoverIdleFrames[0] = 0;
    g_TutorialRecoverIdleFrames[1] = 0;

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
    g_BattlePlayer1Won = true;
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

void UpdateBattleFlow() {
    int steps = g_GameTimer.GetLastFramesToUpdate();
    if (steps <= 0) steps = 1;
    if (steps > GAME_TIMER_MAX_STEPS_PER_FRAME) steps = GAME_TIMER_MAX_STEPS_PER_FRAME;

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
            g_BattleTimeRemainingSteps -= steps;
            if (g_BattleTimeRemainingSteps < 0) {
                g_BattleTimeRemainingSteps = 0;
            }
            if (g_BattleTimeRemainingSteps == 0 && g_Player1 && g_Player2) {
                const int p1Hp = g_Player1->GetHealth();
                const int p2Hp = g_Player2->GetHealth();
                if (p1Hp > p2Hp) {
                    g_BattlePlayer1Won = true;
                }
                else if (p2Hp > p1Hp) {
                    g_BattlePlayer1Won = false;
                }
                else {
                    g_BattlePlayer1Won = true;
                }
                g_BattleFlowPhase = BattleFlowPhase::Knockout;
                g_BattleFlowTimer = 0;
                break;
            }
        }

        if (g_Player1 && g_Player2 && (g_Player1->IsDead() || g_Player2->IsDead())) {
            if (IsTutorialBattleMode()) {
                break;
            }
            g_BattlePlayer1Won = g_Player2->IsDead() && !g_Player1->IsDead();
            if (g_Player1->IsDead() && g_Player2->IsDead()) {
                g_BattlePlayer1Won = false;
            }
            g_BattleFlowPhase = BattleFlowPhase::Knockout;
            g_BattleFlowTimer = 0;
        }
        break;

    case BattleFlowPhase::Knockout:
        g_BattleFlowTimer += steps;
        if (g_BattleFlowTimer >= BATTLE_KO_HOLD_STEPS) {
            if (g_Player1 && g_Player2) {
                if (g_BattlePlayer1Won) {
                    g_Player1->BeginVictoryPose();
                    g_Player2->BeginDefeatPose();
                }
                else {
                    g_Player2->BeginVictoryPose();
                    g_Player1->BeginDefeatPose();
                }
            }
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
        break;
    }
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
