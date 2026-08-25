#include "tutorial_guide.h"
#include "game_logic.h"
#include "input.h"
#include "player/CharacterId.h"
#include "player/makoto/Makoto.h"
#include "player/joker/Joker.h"
#include "player/narukami/Narukami.h"
#include "player/yosuke/Yosuke.h"
#include <cmath>

struct TutorialActionStep {
    const char* title;
    const char* detail;
    bool moveStep;
    int states[14];
};

static int g_TutorialStepIndex = 0;
static int g_TutorialStepCount = 0;
static const TutorialActionStep* g_TutorialSteps = nullptr;
static float g_TutorialHumanStartX = 0.0f;
static bool g_TutorialGuideReady = false;

static const char* kTutorialFreeTitle = "Free Practice";
static const char* kTutorialFreeDetail =
    "All moves complete! HP and SP refill in Tutorial. Keep training on the sandbag.";

#define TUTORIAL_END -1

static const TutorialActionStep kMakotoTutorialSteps[] = {
    { "Move", "A/D: Walk or run toward the sandbag.", true, { TUTORIAL_END } },
    { "Basic Attack", "LMB: Neutral attack.", false, { ATTACK, TUTORIAL_END } },
    { "Side Attack", "E: Side attack (Stamina).", false, { SIDE_ATTACK, TUTORIAL_END } },
    { "Up Attack", "R: Up attack (Stamina).", false, { ATTACK_UP, TUTORIAL_END } },
    { "Down Attack", "S + LMB: Down attack.", false, { DOWN_ATTACK, TUTORIAL_END } },
    { "Jump", "Space: Jump.", false, { JUMP, TUTORIAL_END } },
    { "Dash", "J: Dash (Stamina).", false, { DASH, TUTORIAL_END } },
    { "Dodge", "RMB: Dodge forward or back.", false, { DODGE_FORWARD, DODGE_BACKWARD, TUTORIAL_END } },
    { "Guard", "Hold A/D away from the sandbag + S to guard (Shift = Run).", false, { GUARD, GUARD_AIR, TUTORIAL_END } },
    { "Crouch", "C: Crouch.", false, { CROUCH, TUTORIAL_END } },
    { "Crouch Attack", "C + LMB: Crouch attack.", false, { CROUCH_ATTACK, TUTORIAL_END } },
    { "Air Neutral", "LMB in the air.", false, { NEUTRAL_AIR, TUTORIAL_END } },
    { "Air Side", "Space + E: Air side attack.", false, { SIDE_AIR, TUTORIAL_END } },
    { "Air Up", "Space + R: Air up attack.", false, { UP_AIR, TUTORIAL_END } },
    { "Air Down", "Air down attack.", false, { DOWN_AIR, TUTORIAL_END } },
    { "Persona 1", "1: Orpheus / Agi.", false, { SUMMON_1, SUMMON_1_ORPHEUS, TUTORIAL_END } },
    { "Persona 2", "2: Jack Frost / Mabufu.", false, { SUMMON_2, SUMMON_2_JACKFROST, TUTORIAL_END } },
    { "Air Persona 3", "Space + 3: Thanatos / Maziodyne.", false, { SUMMON_AIR, SUMMON_AIR_MAZIODYNE, TUTORIAL_END } },
    { "Air Persona 4", "Space + 4: Messiah / Megidolaon.", false, { SUMMON_AIR_2, SUMMON_AIR_MESSIAH, TUTORIAL_END } },
    { "Ultimate 5", "5: Thanatos Slash.", false, { THANATOS_SLASH, TUTORIAL_END } },
    { "Taunt", "T: Taunt.", false, { TAUNT, TUTORIAL_END } },
};

static const TutorialActionStep kJokerTutorialSteps[] = {
    { "Move", "A/D: Walk or run.", true, { TUTORIAL_END } },
    { "Basic Attack", "LMB: Neutral attack.", false, { JOKER_ATTACK, TUTORIAL_END } },
    { "Forward Attack", "E: Forward attack (Stamina).", false, { JOKER_FORWARD_ATTACK, TUTORIAL_END } },
    { "Up Attack", "R: Up attack (Stamina).", false, { JOKER_UP_ATTACK, TUTORIAL_END } },
    { "Down Attack", "S + LMB: Down attack.", false, { JOKER_DOWN_ATTACK, TUTORIAL_END } },
    { "Forward Smash", "Shift + E: Forward smash.", false, { JOKER_FORWARD_SMASH, TUTORIAL_END } },
    { "Up Smash", "Shift + R: Up smash.", false, { JOKER_UP_SMASH, TUTORIAL_END } },
    { "Down Smash", "Shift + S + LMB: Down smash.", false, { JOKER_DOWN_SMASH, TUTORIAL_END } },
    { "Jump", "Space: Jump.", false, { JOKER_JUMP, TUTORIAL_END } },
    { "Dash", "J: Dash (Stamina).", false, { JOKER_DASH, TUTORIAL_END } },
    { "Dodge", "RMB: Dodge.", false, { JOKER_DODGE, TUTORIAL_END } },
    { "Ledgeroll", "Shift + RMB: Ledgeroll.", false, { JOKER_LEDGEROLL, TUTORIAL_END } },
    { "Guard", "Hold A/D away from the sandbag + S to guard (Shift = Run).", false, { JOKER_GUARD, JOKER_GUARD_AIR, TUTORIAL_END } },
    { "Air Neutral", "LMB in the air.", false, { JOKER_NEUTRAL_AIR, TUTORIAL_END } },
    { "Air Forward", "E in the air (Stamina).", false, { JOKER_FORWARD_AIR, TUTORIAL_END } },
    { "Air Back", "Jump, hold A away from foe, tap E.", false, { JOKER_BACK_AIR, TUTORIAL_END } },
    { "Air Up", "R in the air (Stamina).", false, { JOKER_UP_AIR, TUTORIAL_END } },
    { "Air Down", "S in the air (Stamina).", false, { JOKER_DOWN_AIR, TUTORIAL_END } },
    { "Eiha", "1: Eiha (summon Arsene, then curse).", false, { JOKER_PERSONA_SUMMON, JOKER_EIHA, TUTORIAL_END } },
    { "Eigaon", "2: Eigaon (summon Arsene, then heavy curse).", false, { JOKER_PERSONA_SUMMON, JOKER_EIGAON, TUTORIAL_END } },
    { "Neutral Special", "3: Neutral special.", false, { JOKER_NEUTRAL_SPECIAL, JOKER_NEUTRAL_AIR_SPECIAL, TUTORIAL_END } },
    { "All-Out Attack", "4: All-Out Attack.", false, {
        JOKER_ALL_OUT_ATTACK, JOKER_ALL_OUT_MEMBER, JOKER_ALL_OUT_EFFECT, JOKER_ALL_OUT_FINISH, TUTORIAL_END } },
    { "Taunt", "T: Taunt.", false, { JOKER_TAUNT, TUTORIAL_END } },
};

static const TutorialActionStep kNarukamiTutorialSteps[] = {
    { "Move", "A/D: Walk or run.", true, { TUTORIAL_END } },
    { "Basic Attack", "LMB: Neutral attack.", false, { NARUKAMI_ATTACK, TUTORIAL_END } },
    { "Swift Strike", "S + LMB: Swift strike.", false, { NARUKAMI_SIDE_ATTACK, TUTORIAL_END } },
    { "Cross Slash", "R: Cross slash.", false, { NARUKAMI_ATTACK_UP, TUTORIAL_END } },
    { "Lightning Flash", "G: Lightning flash (down).", false, { NARUKAMI_DOWN_ATTACK, TUTORIAL_END } },
    { "Raging Lion", "W + LMB: Raging Lion.", false, { NARUKAMI_RAGING_LION, TUTORIAL_END } },
    { "Big Gamble", "E: Big Gamble.", false, { NARUKAMI_BIG_GAMBLE, TUTORIAL_END } },
    { "Crouch", "C: Crouch.", false, { NARUKAMI_CROUCH, TUTORIAL_END } },
    { "Crouch Attack", "C + LMB: Crouch attack.", false, { NARUKAMI_CROUCH_ATTACK, TUTORIAL_END } },
    { "Jump", "Space: Jump.", false, { NARUKAMI_JUMP, TUTORIAL_END } },
    { "Dash", "Hold J: Dash.", false, { NARUKAMI_DASH, TUTORIAL_END } },
    { "Guard", "Hold A/D away from the sandbag + S to guard (Shift = Run).", false, { NARUKAMI_GUARD, NARUKAMI_GUARD_AIR, TUTORIAL_END } },
    { "Air Attack", "Space + LMB: Air combo.", false, { NARUKAMI_NEUTRAL_AIR, TUTORIAL_END } },
    { "Zio", "1: Zio summon.", false, { NARUKAMI_SUMMON_ZIO, TUTORIAL_END } },
    { "Ziodyne", "2: Ziodyne summon.", false, { NARUKAMI_SUMMON_ZIODYNE, TUTORIAL_END } },
    { "Persona Ground", "3: Persona ground attack.", false, { NARUKAMI_PERSONA_SUMMON, TUTORIAL_END } },
    { "Persona Air", "4: Persona air attack.", false, { NARUKAMI_PERSONA_AIR_SUMMON, TUTORIAL_END } },
    { "Myriad Truths", "5: Myriad Truths.", false, { NARUKAMI_MYRIAD_TRUTHS, TUTORIAL_END } },
    { "Taunt", "T: Taunt.", false, { NARUKAMI_TAUNT, TUTORIAL_END } },
};

static const TutorialActionStep kYosukeTutorialSteps[] = {
    { "Move", "A/D: Walk or run.", true, { TUTORIAL_END } },
    { "Basic Attack", "LMB: Neutral attack.", false, { YOSUKE_ATTACK, TUTORIAL_END } },
    { "Crescent Slash", "R: Crescent Slash.", false, { YOSUKE_CRESCENT_SLASH, TUTORIAL_END } },
    { "Moonsault", "E: Moonsault.", false, { YOSUKE_MOONSAULT, TUTORIAL_END } },
    { "Flying Kunai", "Space + E: Flying Kunai.", false, { YOSUKE_FLYING_KUNAI, TUTORIAL_END } },
    { "Dash", "Hold J: Forward dash.", false, { YOSUKE_DASH, TUTORIAL_END } },
    { "Back Dash", "RMB + away: Back dash.", false, { YOSUKE_BACK_DASH, TUTORIAL_END } },
    { "Jump", "Space: Jump.", false, { YOSUKE_JUMP, TUTORIAL_END } },
    { "Guard", "Hold A/D away from the sandbag + S to guard (Shift = Run).", false, { YOSUKE_GUARD, YOSUKE_GUARD_AIR, TUTORIAL_END } },
    { "Air Combo", "LMB in the air.", false, { YOSUKE_AIR_COMBO, TUTORIAL_END } },
    { "Persona Summon", "1: Jiraiya strike at foe's front.", false, { YOSUKE_PERSONA_SUMMON, YOSUKE_PERSONA_JIRAIYA, TUTORIAL_END } },
    { "Mirage Slash", "2: Mirage Slash.", false, { YOSUKE_MIRAGE_SLASH, TUTORIAL_END } },
    { "Brave Blade", "3: Brave Blade (Jiraiya at foe).", false, { YOSUKE_BRAVE_BLADE, TUTORIAL_END } },
    { "Garudyne", "4: Garudyne (spin in, Jiraiya behind).", false, { YOSUKE_GARUDYNE, TUTORIAL_END } },
};

static Fighter* GetTutorialHumanFighter() {
    if (!g_Player1 || !g_Player2) return nullptr;
    if (g_Player1->IsHumanControlled()) return g_Player1;
    if (g_Player2->IsHumanControlled()) return g_Player2;
    return g_Player1;
}

static void BindTutorialStepsForCharacter(CharacterId id) {
    switch (id) {
    case Char_Makoto:
        g_TutorialSteps = kMakotoTutorialSteps;
        g_TutorialStepCount = (int)(sizeof(kMakotoTutorialSteps) / sizeof(kMakotoTutorialSteps[0]));
        break;
    case Char_Joker:
        g_TutorialSteps = kJokerTutorialSteps;
        g_TutorialStepCount = (int)(sizeof(kJokerTutorialSteps) / sizeof(kJokerTutorialSteps[0]));
        break;
    case Char_Narukami:
        g_TutorialSteps = kNarukamiTutorialSteps;
        g_TutorialStepCount = (int)(sizeof(kNarukamiTutorialSteps) / sizeof(kNarukamiTutorialSteps[0]));
        break;
    case Char_Yosuke:
        g_TutorialSteps = kYosukeTutorialSteps;
        g_TutorialStepCount = (int)(sizeof(kYosukeTutorialSteps) / sizeof(kYosukeTutorialSteps[0]));
        break;
    default:
        g_TutorialSteps = kMakotoTutorialSteps;
        g_TutorialStepCount = (int)(sizeof(kMakotoTutorialSteps) / sizeof(kMakotoTutorialSteps[0]));
        break;
    }
}

static bool TutorialStateMatches(int actionState, const TutorialActionStep& step) {
    for (int i = 0; i < (int)(sizeof(step.states) / sizeof(step.states[0])); ++i) {
        const int target = step.states[i];
        if (target == TUTORIAL_END) break;
        if (actionState == target) return true;
    }
    return false;
}

static bool TutorialMoveStepComplete(Fighter& human) {
    const int state = human.GetActionState();
    if (state == STANCE || state == WALK || state == RUN ||
        state == JOKER_STAND || state == JOKER_WALK || state == JOKER_RUN ||
        state == NARUKAMI_STANCE || state == NARUKAMI_WALK || state == NARUKAMI_RUN ||
        state == YOSUKE_STANCE || state == YOSUKE_WALK || state == YOSUKE_RUN) {
        if (IsGameKeyDown(DIK_LEFT) || IsGameKeyDown(DIK_A) ||
            IsGameKeyDown(DIK_RIGHT) || IsGameKeyDown(DIK_D)) {
            return true;
        }
    }
    return fabsf(human.position.x - g_TutorialHumanStartX) > 36.0f;
}

void ResetTutorialGuide() {
    g_TutorialStepIndex = 0;
    g_TutorialHumanStartX = 0.0f;
    g_TutorialGuideReady = false;
    g_TutorialSteps = nullptr;
    g_TutorialStepCount = 0;

    if (Fighter* human = GetTutorialHumanFighter()) {
        BindTutorialStepsForCharacter(human->GetCharacterId());
    }
}

const char* GetTutorialGuideObjective() {
    if (!g_TutorialSteps || g_TutorialStepIndex >= g_TutorialStepCount) {
        return kTutorialFreeTitle;
    }
    return g_TutorialSteps[g_TutorialStepIndex].title;
}

const char* GetTutorialGuideDetail() {
    if (!g_TutorialSteps || g_TutorialStepIndex >= g_TutorialStepCount) {
        return kTutorialFreeDetail;
    }
    return g_TutorialSteps[g_TutorialStepIndex].detail;
}

int GetTutorialGuideStepIndex() {
    if (!g_TutorialSteps || g_TutorialStepCount <= 0) return 0;
    if (g_TutorialStepIndex >= g_TutorialStepCount) return g_TutorialStepCount;
    return g_TutorialStepIndex;
}

int GetTutorialGuideStepCount() {
    return g_TutorialStepCount > 0 ? g_TutorialStepCount : 1;
}

bool IsTutorialGuideComplete() {
    return g_TutorialSteps != nullptr &&
        g_TutorialStepCount > 0 &&
        g_TutorialStepIndex >= g_TutorialStepCount;
}

void UpdateTutorialGuide(int steps) {
    (void)steps;
    if (!IsTutorialBattleMode() || !IsBattleCombatActive() || IsBattleEndSequence()) return;

    Fighter* human = GetTutorialHumanFighter();
    if (!human) return;

    if (!g_TutorialSteps || g_TutorialStepCount <= 0) {
        BindTutorialStepsForCharacter(human->GetCharacterId());
    }
    if (!g_TutorialSteps || g_TutorialStepCount <= 0) return;

    if (!g_TutorialGuideReady) {
        g_TutorialHumanStartX = human->position.x;
        g_TutorialGuideReady = true;
    }

    if (g_TutorialStepIndex >= g_TutorialStepCount) return;

    const TutorialActionStep& step = g_TutorialSteps[g_TutorialStepIndex];
    bool completed = false;
    if (step.moveStep) {
        completed = TutorialMoveStepComplete(*human);
    }
    else {
        completed = TutorialStateMatches(human->GetActionState(), step);
    }

    if (completed) {
        ++g_TutorialStepIndex;
        if (g_TutorialStepIndex < g_TutorialStepCount) {
            g_TutorialHumanStartX = human->position.x;
        }
    }
}
