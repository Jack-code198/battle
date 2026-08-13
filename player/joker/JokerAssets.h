#pragma once

// Joker Asset Module (BMCS2224)
// Joker + Arsene animation catalog indices (256x256 sprite sheets).
// pairedWithArsene=true means Joker and Arsene play in sync for stance/move.
// arseneFrameCount: 0 = use frameCount; otherwise Arsene sheet uses its own length
// (needed when Arsene has fewer cells than Joker, e.g. walk/run → arsene_run).

enum JokerAnimId {
    JOKER_ANIM_STANCE,
    JOKER_ANIM_IDLE,
    JOKER_ANIM_WALK,
    JOKER_ANIM_RUN,
    JOKER_ANIM_DASH,
    JOKER_ANIM_JUMP,
    JOKER_ANIM_DODGE,
    JOKER_ANIM_GUARD,
    JOKER_ANIM_GUARD_AIR,
    JOKER_ANIM_ATTACK_COMBO,
    JOKER_ANIM_FORWARD_ATTACK,
    JOKER_ANIM_UP_ATTACK,
    JOKER_ANIM_DOWN_ATTACK,
    JOKER_ANIM_FORWARD_SMASH,
    JOKER_ANIM_UP_SMASH,
    JOKER_ANIM_DOWN_SMASH,
    JOKER_ANIM_NEUTRAL_AIR,
    JOKER_ANIM_FORWARD_AIR,
    JOKER_ANIM_BACK_AIR,
    JOKER_ANIM_DOWN_AIR,
    JOKER_ANIM_UP_AIR,
    JOKER_ANIM_NEUTRAL_SPECIAL,
    JOKER_ANIM_NEUTRAL_AIR_SPECIAL,
    JOKER_ANIM_EIHA,
    JOKER_ANIM_EIGAON,
    JOKER_ANIM_ALL_OUT_ATTACK,
    JOKER_ANIM_ALL_OUT_MEMBER,
    JOKER_ANIM_ALL_OUT_EFFECT,
    JOKER_ANIM_ALL_OUT_FINISH,
    JOKER_ANIM_PERSONA_SUMMON,
    JOKER_ANIM_PERSONA_RETURN,
    JOKER_ANIM_INTRO,
    JOKER_ANIM_TAUNT,
    JOKER_ANIM_DAMAGE,
    JOKER_ANIM_RECOVER,
    JOKER_ANIM_LEDGEROLL,
    JOKER_ANIM_WIN,
    JOKER_ANIM_LOSE,
    JOKER_ANIM_COUNT
};

struct JokerAnimInfo {
    const char* jokerFile;
    const char* arseneFile;
    const char* jokerEffectFile;
    const char* arseneEffectFile;
    int frameCount;
    int arseneFrameCount;
    bool pairedWithArsene;
};

// Asset table used by Joker loading (and future AI / input expansion).
// frameCount matches nonempty 256px cells (empty trailing cells cause flicker).
inline constexpr JokerAnimInfo kJokerAnimCatalog[JOKER_ANIM_COUNT] = {
    { "stance.png", "arsene_stance.png", nullptr, nullptr, 4, 0, true },
    { "idle.png", nullptr, nullptr, nullptr, 7, 0, false },
    { "walk.png", "arsene_run.png", nullptr, nullptr, 8, 3, true },
    { "run.png", "arsene_run.png", nullptr, nullptr, 8, 3, true },
    { "dash.png", "arsene_dash.png", nullptr, nullptr, 5, 4, true },
    { "jump.png", "arsene_jump.png", nullptr, nullptr, 8, 6, true },
    { "dodge.png", nullptr, nullptr, nullptr, 6, 0, false },
    { "guard.png", "arsene_guard.png", nullptr, nullptr, 3, 3, true },
    { "guard_air.png", nullptr, nullptr, nullptr, 3, 0, false },
    { "attack_combo.png", "aresene_attack_combo.png", nullptr, nullptr, 11, 10, true },
    { "forward_attack.png", "arsene_forward_attack.png", nullptr, nullptr, 5, 0, true },
    { "up_attack.png", "arsene_up_attack.png", "up_attack_effect.png", "arsene_up_attack_effect.png", 6, 4, true },
    { "down_attack.png", "arsene_down_attack.png", nullptr, nullptr, 5, 0, true },
    { "forward_smash.png", "arsene_forward_smash.png", nullptr, "arsene_forward_smash_effect.png", 4, 3, true },
    { "up_smash.png", "arsene_up_smash.png", nullptr, "arsene_up_smash_effect.png", 6, 3, true },
    { "down_smash.png", "arsene_down_smash.png", nullptr, nullptr, 5, 4, true },
    { "neutral_air.png", "arsene_neutral_air.png", nullptr, nullptr, 5, 0, true },
    { "forward_air_attack.png", "arsene_forward_air_attack.png", nullptr, nullptr, 5, 0, true },
    { "back_air_attack.png", "arsene_back_air_attack.png", nullptr, "arsene_back_air_attack_effect.png", 3, 0, true },
    { "down_air_attack.png", "aresene_down_air_attack.png", nullptr, nullptr, 5, 3, true },
    { "up_air_attack.png", "arsene_up_air_attack.png", nullptr, "arsene_up_air_attack_effect.png", 6, 3, true },
    { "neutral_special.png", nullptr, "neutral_special_effect.png", nullptr, 16, 0, false },
    { "neutral_air_special.png", nullptr, nullptr, nullptr, 8, 0, false },
    { "eiha.png", "arsene_eiha.png", "eiha_effect.png", nullptr, 5, 4, true },
    { "eigaon.png", "arsene_eigaon.png", "eigaon_effect.png", nullptr, 5, 4, true },
    { "all-out_attack.png", nullptr, nullptr, nullptr, 9, 0, false },
    // Sheet is 4 cells wide but only cell 0 has the member portraits.
    { "all-out_attack_member.png", nullptr, nullptr, nullptr, 1, 0, false },
    { "all-out_attack_effect.png", nullptr, nullptr, nullptr, 10, 0, false },
    { "all-out_attack_finish.png", nullptr, nullptr, nullptr, 5, 0, false },
    { "persona!.png", nullptr, nullptr, nullptr, 4, 0, false },
    { "persona_return.png", nullptr, nullptr, nullptr, 4, 0, false },
    { "intro.png", nullptr, nullptr, nullptr, 7, 0, false },
    { "taunt.png", "arsene_taunt.png", "mona_taunt.png", nullptr, 8, 0, true },
    { "damage.png", nullptr, nullptr, nullptr, 6, 0, false },
    { "recover.png", nullptr, nullptr, nullptr, 3, 0, false },
    { "ledgeroll.png", nullptr, nullptr, nullptr, 8, 0, false },
    { "win.png", nullptr, nullptr, nullptr, 12, 0, false },
    { "lose.png", nullptr, nullptr, nullptr, 8, 0, false },
};
