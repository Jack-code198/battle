#pragma once

// Joker + Arsene animation catalog indices (256x256 sprite sheets).
// pairedWithArsene=true means Joker and Arsene play in sync for stance/move.
enum JokerAnimId {
    JOKER_ANIM_STANCE,
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
    JOKER_ANIM_PERSONA_SUMMON,
    JOKER_ANIM_PERSONA_RETURN,
    JOKER_ANIM_INTRO,
    JOKER_ANIM_TAUNT,
    JOKER_ANIM_DAMAGE,
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
    bool pairedWithArsene;
};

// Asset table used by Joker loading (and future AI / input expansion).
inline constexpr JokerAnimInfo kJokerAnimCatalog[JOKER_ANIM_COUNT] = {
    { "stance.png", "arsene_stance.png", nullptr, nullptr, 4, true },
    { "walk.png", "arsene_run.png", nullptr, nullptr, 8, true },
    { "run.png", "arsene_run.png", nullptr, nullptr, 8, true },
    { "dash.png", "arsene_dash.png", nullptr, nullptr, 8, true },
    { "jump.png", "arsene_jump.png", nullptr, nullptr, 8, true },
    { "dodge.png", nullptr, nullptr, nullptr, 4, false },
    { "guard.png", "arsene_guard.png", nullptr, nullptr, 4, true },
    { "guard_air.png", nullptr, nullptr, nullptr, 4, false },
    { "attack_combo.png", "aresene_attack_combo.png", nullptr, nullptr, 12, true },
    { "forward_attack.png", "arsene_forward_attack.png", nullptr, nullptr, 8, true },
    { "up_attack.png", "arsene_up_attack.png", "up_attack_effect.png", "arsene_up_attack_effect.png", 8, true },
    { "down_attack.png", "arsene_down_attack.png", nullptr, nullptr, 8, true },
    { "forward_smash.png", "arsene_forward_smash.png", nullptr, "arsene_forward_smash_effect.png", 4, true },
    { "up_smash.png", "arsene_up_smash.png", nullptr, "arsene_up_smash_effect.png", 8, true },
    { "down_smash.png", "arsene_down_smash.png", nullptr, nullptr, 8, true },
    { "neutral_air.png", "arsene_neutral_air.png", nullptr, nullptr, 8, true },
    { "forward_air_attack.png", "arsene_forward_air_attack.png", nullptr, nullptr, 8, true },
    { "back_air_attack.png", "arsene_back_air_attack.png", nullptr, "arsene_back_air_attack_effect.png", 4, true },
    { "down_air_attack.png", "aresene_down_air_attack.png", nullptr, nullptr, 4, true },
    { "up_air_attack.png", "arsene_up_air_attack.png", nullptr, "arsene_up_air_attack_effect.png", 8, true },
    { "neutral_special.png", nullptr, "neutral_special_effect.png", nullptr, 16, false },
    { "neutral_air_special.png", nullptr, nullptr, nullptr, 8, false },
    { "eiha.png", "arsene_eiha.png", "eiha_effect.png", nullptr, 8, true },
    { "eigaon.png", "arsene_eigaon.png", "eigaon_effect.png", nullptr, 8, true },
    { "all-out_attack.png", nullptr, nullptr, nullptr, 28, false },
    { "persona!.png", nullptr, nullptr, nullptr, 4, false },
    { "persona_return.png", nullptr, nullptr, nullptr, 4, false },
    { "intro.png", nullptr, nullptr, nullptr, 8, false },
    { "taunt.png", "arsene_taunt.png", nullptr, nullptr, 8, true },
    { "damage.png", nullptr, nullptr, nullptr, 3, false },
    { "ledgeroll.png", nullptr, nullptr, nullptr, 8, false },
    { "win.png", nullptr, nullptr, nullptr, 12, false },
    { "lose.png", nullptr, nullptr, nullptr, 8, false },
};
