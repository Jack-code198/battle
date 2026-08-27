#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <dinput.h>
#include <cmath>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

#include "collision.h"

// window / screen
extern HWND g_hWnd;
extern WNDCLASS wndClass;
extern MSG msg;
extern bool g_WindowHasFocus;
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;

// Character render scale / physics
inline constexpr float CHARACTER_SCREEN_HEIGHT_RATIO = 0.22f;
inline constexpr float MAKOTO_SCREEN_HEIGHT_RATIO = 0.24f;
inline constexpr float CHARACTER_REFERENCE_HEIGHT = 64.0f;
inline constexpr float CHARACTER_REFERENCE_WIDTH = 48.0f;
inline constexpr float MAKOTO_BODY_HEIGHT = 56.0f;
inline constexpr float MAKOTO_FEET_Y = 56.0f;
// Shared sole row for crouch hold + crouch attack (measured from sheets).
inline constexpr float MAKOTO_CROUCH_FEET_Y = 42.0f;
inline constexpr float MAKOTO_BODY_WIDTH = 26.0f;
inline constexpr float MAKOTO_BODY_CENTER_X = 11.0f;
inline constexpr float MAKOTO_WINDOW_MARGIN = 12.0f;
inline constexpr float MAKOTO_SPAWN_FORWARD = 200.0f;
inline constexpr float JOKER_SPAWN_X = 680.0f;
inline constexpr float OPPONENT_SPAWN_X = JOKER_SPAWN_X;
inline constexpr float OPPONENT_SKILL_ANCHOR_RATIO = 0.45f;

inline float GetBattleGroundY() {
    return (float)SCREEN_HEIGHT * (980.0f / 1080.0f);
}
#define CHARACTER_GROUND_Y GetBattleGroundY()
inline constexpr float GRAVITY = 0.55f;
inline constexpr float MESSIAH_REFERENCE_HEIGHT = 152.0f;
inline constexpr float MEGIDOLAON_REFERENCE_HEIGHT = 176.0f;
inline constexpr float THANATOS_MAZIODYNE_FORWARD_OFFSET = 50.0f;
inline constexpr float THANATOS_MAZIODYNE_VERTICAL_OFFSET = 350.0f;
inline constexpr float MAZIODYNE_HORIZONTAL_OFFSET = 50.0f;
inline constexpr float MAZIODYNE_VERTICAL_OFFSET = 35.0f;
inline constexpr float PERSONA_BEHIND_HORIZONTAL = 90.0f;
inline constexpr float ORPHEUS_BEHIND_HORIZONTAL = 145.0f;
inline constexpr float JACKFROST_BEHIND_VERTICAL = 110.0f;
inline constexpr float ORPHEUS_BEHIND_VERTICAL = 130.0f;
inline constexpr float MESSIAH_HORIZONTAL_OFFSET = 350.0f;
inline constexpr float MESSIAH_VERTICAL_OFFSET = 300.0f;
inline constexpr float MEGIDOLAON_HORIZONTAL_OFFSET = 700.0f;
inline constexpr float MEGIDOLAON_VERTICAL_OFFSET = 500.0f;
inline constexpr float THANATOS_SLASH_HORIZONTAL_OFFSET = 0.0f;
inline constexpr float THANATOS_SLASH_VERTICAL_OFFSET = -70.0f;
inline constexpr float DODGE_SLIDE_SPEED = 2.5f;
inline constexpr float OPPONENT_RETURN_SPEED = 8.0f;
inline constexpr int OPPONENT_STAND_ANIM_TICKS = 8;
inline constexpr int OPPONENT_WALK_ANIM_TICKS = 6;
inline constexpr int OPPONENT_DAMAGE_ANIM_TICKS = 5;
inline constexpr int JOKER_INTRO_TICKS = 6;
inline constexpr int JOKER_IDLE_ANIM_TICKS = 8;
// Recover only has 3 frames — give each frame enough time to read.
inline constexpr int JOKER_RECOVER_ANIM_TICKS = 7;
// Damage / knockdown timing.
inline constexpr int JOKER_DAMAGE_ANIM_TICKS = 4;
inline constexpr int JOKER_DAMAGE_GROUND_HOLD_TICKS = 10;
// Same wait idea as Makoto before leaving stance for idle (~10s at 60fps).
inline constexpr int JOKER_IDLE_WAIT_FRAMES = 600;

inline constexpr float JOKER_BODY_WIDTH = 22.0f;
inline constexpr float JOKER_HURTBOX_WIDTH = 26.0f;
inline constexpr float JOKER_HURTBOX_HEIGHT = 56.0f;
inline constexpr float JOKER_PUSHBOX_WIDTH = 26.0f;
inline constexpr float JOKER_PUSHBOX_HEIGHT = 52.0f;

// Default fighter hurtbox in unscaled body units (scaled by GetCharacterRenderScale()).
// Sized from Makoto's visible body silhouette in the 256px cell sheet.
inline constexpr float DEFAULT_HURTBOX_WIDTH = 36.0f;
inline constexpr float DEFAULT_HURTBOX_HEIGHT = 110.0f;

// Movement speeds (pixels per logic step before facing direction).
inline constexpr int MAKOTO_MOVE_SPEED = 6;
inline constexpr int NARUKAMI_MOVE_SPEED = 6;
// Same units as MAKOTO_BODY_WIDTH (scaled by GetMakotoDrawScale() — Narukami draw uses that).
inline constexpr float NARUKAMI_PUSHBOX_WIDTH = 36.0f;
inline constexpr float NARUKAMI_PUSHBOX_HEIGHT = MAKOTO_BODY_HEIGHT;
inline constexpr int JOKER_MOVE_SPEED = 6;
inline constexpr float MAKOTO_DASH_SPEED_MULTIPLIER = 3.5f;

// Joker vertical clamp (keeps sandbag opponent near the battle ground band).
inline constexpr float JOKER_MAX_GROUND_SLACK = 20.0f;
inline constexpr float JOKER_MIN_SCREEN_Y = 200.0f;

// Ground contact tolerances (shared physics snap).
inline constexpr float GROUND_CONTACT_EPSILON = 0.5f;
inline constexpr float GROUND_NEAR_EPSILON = 2.0f;
inline constexpr float BODY_COLLISION_EPSILON = 0.5f;
inline constexpr float BODY_MOVE_SUBSTEP = 10.0f;

inline void PinFighterToGround(D3DXVECTOR3& position, float& verticalVelocity) {
    position.y = CHARACTER_GROUND_Y;
    verticalVelocity = 0.0f;
}

inline bool IsFighterAtGroundLevel(const D3DXVECTOR3& position) {
    return position.y >= CHARACTER_GROUND_Y - GROUND_CONTACT_EPSILON;
}

inline bool IsFighterPinnedToGround(const D3DXVECTOR3& position, float verticalVelocity) {
    return IsFighterAtGroundLevel(position) && verticalVelocity >= 0.0f;
}

inline float SampleFeetYTable(const float* values, int count, int frameIndex) {
    if (!values || count <= 0) return 0.0f;
    if (frameIndex < 0) frameIndex = 0;
    if (frameIndex >= count) frameIndex = count - 1;
    return values[frameIndex];
}

// Measured sole/back contact rows inside each 256px damage/recover cell.
// Knockdown lying frames sit near the TOP of the cell — use low row values (~100-140).
inline constexpr float MAKOTO_DAMAGE_FEET_Y[] = { 115.0f, 128.0f, 138.0f };
inline constexpr float MAKOTO_RECOVER_FEET_Y_TABLE[] = { 82.0f, 96.0f, 112.0f, 138.0f };
inline constexpr float MAKOTO_KNOCKDOWN_GROUND_FEET_Y = 138.0f;

inline constexpr float JOKER_DAMAGE_FEET_Y[] = { 228.0f, 205.0f, 188.0f, 172.0f, 158.0f, 108.0f };
inline constexpr float JOKER_RECOVER_FEET_Y_TABLE[] = { 238.0f, 198.0f, 56.0f };
inline constexpr float JOKER_KNOCKDOWN_GROUND_FEET_Y = 108.0f;

inline constexpr float NARUKAMI_DAMAGE_FEET_Y[] = { 228.0f, 210.0f, 198.0f, 185.0f, 172.0f, 128.0f, 108.0f };
inline constexpr float NARUKAMI_RECOVER_FEET_Y_TABLE[] = { 238.0f, 212.0f, 56.0f };
inline constexpr float NARUKAMI_KNOCKDOWN_GROUND_FEET_Y = 108.0f;

inline constexpr float YOSUKE_DAMAGE_FEET_Y[] = { 232.0f, 228.0f, 220.0f, 210.0f, 145.0f, 130.0f, 112.0f };
inline constexpr float YOSUKE_RECOVER_FEET_Y_TABLE[] = { 92.0f, 108.0f, 52.0f, 52.0f, 52.0f };
inline constexpr float YOSUKE_KNOCKDOWN_GROUND_FEET_Y = 112.0f;

inline constexpr float FIGHTER_DAMAGE_POP_VELOCITY = -6.5f;

inline void ApplyStandardHitReactionVertical(D3DXVECTOR3& position, float& verticalVelocity, bool /*onGround*/) {
    // Knockdown always plays on the floor — no launch pop.
    PinFighterToGround(position, verticalVelocity);
}

inline float GetGroundedDamageDrawFeetY(const float* airFeetY, int airCount, int /*frameIndex*/, float /*groundedFeetY*/) {
    return SampleFeetYTable(airFeetY, airCount, airCount - 1);
}

// Scan a sprite-sheet frame for the lowest opaque row (after color-key).
// Use as draw feetY so the silhouette bottom sits on CHARACTER_GROUND_Y.
inline float MeasureTextureFrameBottomY(
    LPDIRECT3DTEXTURE9 tex,
    int frameIndex,
    int cellW,
    int cellH,
    int cols)
{
    if (!tex || cellW <= 0 || cellH <= 0 || cols <= 0) {
        return (float)((cellH > 0) ? (cellH - 1) : 0);
    }

    D3DSURFACE_DESC desc;
    if (FAILED(tex->GetLevelDesc(0, &desc))) {
        return (float)(cellH - 1);
    }

    D3DLOCKED_RECT locked;
    if (FAILED(tex->LockRect(0, &locked, NULL, 0))) {
        return (float)(cellH - 1);
    }

    const int col = frameIndex % cols;
    const int row = frameIndex / cols;
    const int xStart = col * cellW;
    const int yStart = row * cellH;
    const int xEnd = (xStart + cellW < (int)desc.Width) ? (xStart + cellW) : (int)desc.Width;
    const int yEnd = (yStart + cellH < (int)desc.Height) ? (yStart + cellH) : (int)desc.Height;

    int bottomLocal = -1;
    for (int y = yEnd - 1; y >= yStart; --y) {
        DWORD* rowPixels = (DWORD*)((BYTE*)locked.pBits + y * locked.Pitch);
        for (int x = xStart; x < xEnd; ++x) {
            if ((rowPixels[x] & 0xFF000000) != 0) {
                bottomLocal = y - yStart;
                break;
            }
        }
        if (bottomLocal >= 0) {
            break;
        }
    }

    tex->UnlockRect(0);
    return (bottomLocal >= 0) ? (float)bottomLocal : (float)(cellH - 1);
}

inline bool TextureFrameHasVisiblePixels(
    LPDIRECT3DTEXTURE9 tex,
    int frameIndex,
    int cellW,
    int cellH,
    int cols)
{
    if (!tex || cellW <= 0 || cellH <= 0 || cols <= 0) {
        return false;
    }

    D3DSURFACE_DESC desc;
    if (FAILED(tex->GetLevelDesc(0, &desc))) {
        return false;
    }

    D3DLOCKED_RECT locked;
    if (FAILED(tex->LockRect(0, &locked, NULL, 0))) {
        return false;
    }

    const int col = frameIndex % cols;
    const int row = frameIndex / cols;
    const int xStart = col * cellW;
    const int yStart = row * cellH;
    const int xEnd = (xStart + cellW < (int)desc.Width) ? (xStart + cellW) : (int)desc.Width;
    const int yEnd = (yStart + cellH < (int)desc.Height) ? (yStart + cellH) : (int)desc.Height;

    bool visible = false;
    for (int y = yStart; y < yEnd && !visible; ++y) {
        DWORD* rowPixels = (DWORD*)((BYTE*)locked.pBits + y * locked.Pitch);
        for (int x = xStart; x < xEnd; ++x) {
            if ((rowPixels[x] & 0xFF000000) != 0) {
                visible = true;
                break;
            }
        }
    }

    tex->UnlockRect(0);
    return visible;
}

// Win sheets often pad with blank tail frames — hold the last frame that has pixels.
inline int FindLastVisibleSheetFrame(
    LPDIRECT3DTEXTURE9 tex,
    int cellW,
    int cellH,
    int cols,
    int frameCount)
{
    if (frameCount <= 0) return 0;
    if (!tex) return frameCount - 1;

    for (int frame = frameCount - 1; frame >= 0; --frame) {
        if (TextureFrameHasVisiblePixels(tex, frame, cellW, cellH, cols)) {
            return frame;
        }
    }
    return frameCount - 1;
}

inline float GetGroundedRecoverDrawFeetY(const float* groundFeetY, int count, int frameIndex) {
    return SampleFeetYTable(groundFeetY, count, frameIndex);
}

// Shared jump / air-control (negative Y = up on screen).
inline constexpr float FIGHTER_JUMP_VELOCITY = -14.0f;
inline constexpr float FIGHTER_AIR_CONTROL_MULTIPLIER = 1.5f;
inline constexpr float NARUKAMI_AIR_CONTROL_MULTIPLIER = 1.2f;
inline constexpr int NARUKAMI_CROSS_SLASH_TICKS = 6;
inline constexpr int NARUKAMI_MYRIAD_SUMMON_TICKS = 16;
inline constexpr int NARUKAMI_MYRIAD_PERSONA_TICKS = 12;
inline constexpr int NARUKAMI_MYRIAD_RIPPLE_MAX_STEPS = 150;
inline constexpr int NARUKAMI_MYRIAD_RIPPLE_END_HOLD_STEPS = 28;
inline constexpr float NARUKAMI_MYRIAD_RIPPLE_VERTICAL = 0.0f;
inline constexpr float NARUKAMI_MYRIAD_IZANAGI_HEAD_GAP = 28.0f;
inline constexpr float NARUKAMI_MYRIAD_RIPPLE_RING_START = 28.0f;
inline constexpr float NARUKAMI_MYRIAD_RIPPLE_RING_GROWTH = 11.0f;
inline constexpr float NARUKAMI_MYRIAD_RIPPLE_RING_SCALE = 42.0f;
inline constexpr int NARUKAMI_HIT_STUN_FRAMES = 20;
// Game loop / frame-timer clamps (named — do not hardcode Sleep/step caps in main).
// Runtime FPS cap, adjustable from the Options screen (60 / 120 / 144).
extern int g_TargetFPS;
inline DWORD GetTargetFrameIntervalMs() {
    if (g_TargetFPS <= 0) return 0;
    return (DWORD)(1000.0 / (double)g_TargetFPS + 0.5);
}
inline constexpr DWORD GAME_LOOP_MIN_FRAME_MS = 0;

// Runtime screen brightness, adjustable via a slider on the Options screen.
inline constexpr int BRIGHTNESS_MIN = 50;
inline constexpr int BRIGHTNESS_MAX = 150;
inline constexpr int BRIGHTNESS_DEFAULT = 100;
inline constexpr int BRIGHTNESS_STEP = 5;
extern int g_BrightnessLevel;
// One logic tick per rendered battle frame (60 Hz @ vsync).
inline constexpr int BATTLE_LOGIC_STEPS_PER_FRAME = 1;
inline constexpr int GAME_TIMER_MAX_STEPS_PER_FRAME = 3;
// Slightly below 1.0 — tiny global gameplay slowdown (~4%) while keeping 1 step/frame smoothness.
inline constexpr float BATTLE_GAMEPLAY_SPEED = 0.96f;

// Simple P2 CPU AI (distances in screen pixels; cooldowns in update steps).
inline constexpr float AI_ATTACK_RANGE = 82.0f;   // legacy engage band (center-ish)
inline constexpr float AI_STRIKE_GAP = 10.0f;     // pushbox gap — must nearly touch to swing
inline constexpr float AI_APPROACH_STOP_GAP = 22.0f;
inline constexpr float AI_SPACING_RANGE = 58.0f;
inline constexpr float AI_ENGAGE_RANGE = 160.0f;
inline constexpr float AI_APPROACH_RANGE = 130.0f;
inline constexpr float AI_RETREAT_RANGE = 50.0f;
inline constexpr float AI_RUN_RANGE = 200.0f;
inline constexpr int AI_ATTACK_COOLDOWN_STEPS = 32;
inline constexpr int AI_ATTACK_PULSE_STEPS = 4;
inline constexpr int AI_SKILL_ATTACK_PULSE_STEPS = 5;
inline constexpr int AI_JUMP_COOLDOWN_STEPS = 140;
inline constexpr int AI_SIDE_ATTACK_CHANCE_PERCENT = 16;
inline constexpr int AI_JUMP_CHANCE_PERCENT = 4;
inline constexpr float AI_GUARD_RANGE = 110.0f;
inline constexpr int AI_APPROACH_BURST_STEPS = 5;
inline constexpr int AI_RETREAT_STEPS = 20;
inline constexpr int AI_IDLE_STEPS = 24;
inline constexpr int AI_MICRO_STEP_STEPS = 10;
inline constexpr int AI_IDLE_CHANCE_PERCENT = 38;
inline constexpr int AI_RETREAT_CHANCE_PERCENT = 22;
inline constexpr int AI_GUARD_REACTION_PERCENT = 50;
inline constexpr int AI_ATTACK_INITIATIVE_PERCENT = 42;
inline constexpr int AI_ENGAGED_ATTACK_PERCENT = 92;
inline constexpr int GUARD_CHIP_DAMAGE = 0;

// Measured recover / air-lift offsets (from sprite feet rows — do not guess).
inline constexpr float MAKOTO_RECOVER_FEET_Y = 32.0f;
inline constexpr float MAKOTO_SPACE_CHORD_AIR_LIFT = 50.0f;
inline constexpr float MAKOTO_SPACE_CHORD_AIR_LIFT_FACTOR = 0.35f;
// Per-frame vertical deltas for Makoto jump sheet (7 frames, measured in Paint).
inline constexpr float MAKOTO_JUMP_FRAME_OFFSETS[7] = {
    0.0f, -16.0f, -24.0f, -32.0f, -14.0f, 14.0f, 48.0f
};
inline constexpr float MAKOTO_JUMP_OFFSET_SCALE = 0.55f;
inline constexpr float NARUKAMI_AIR_LIFT_SHORT = 40.0f;
inline constexpr float NARUKAMI_AIR_LIFT_MID = 45.0f;
inline constexpr float NARUKAMI_AIR_LIFT_TALL = 50.0f;
inline constexpr float JOKER_RECOVER_FEET_Y[3] = { 30.0f, 32.0f, 38.0f };

// HUD / font layout for 1024x768 (named so UI RECTs are not magic literals).
inline constexpr float HUD_NAME_TEXT_WIDTH = 220.0f;
inline constexpr float HUD_NAME_TEXT_HEIGHT = 32.0f;
inline constexpr float FONT_DRAW_LINE_HEIGHT = 64.0f;
inline constexpr float FONT_DRAW_MAX_WIDTH = 800.0f;
inline constexpr float GAME_OVER_TITLE_X = 360.0f;
inline constexpr float GAME_OVER_TITLE_Y = 280.0f;
inline constexpr float GAME_OVER_HINT_X = 300.0f;
inline constexpr float GAME_OVER_HINT_Y = 340.0f;

// Round flow timing (logic steps @ ~60Hz).
inline constexpr int BATTLE_COUNTDOWN_DIGIT_STEPS = 55;
inline constexpr int BATTLE_COUNTDOWN_FIGHT_STEPS = 40;
inline constexpr int BATTLE_KO_HOLD_STEPS = 70;
// Win/lose poses stay on screen after the KO banner until fade begins.
inline constexpr int BATTLE_RESULT_POSE_MIN_STEPS = 150;
inline constexpr int BATTLE_FADE_OUT_STEPS = 54;
// Keep full black on screen briefly before leaving the battle scene.
inline constexpr int BATTLE_FINISHED_BLACK_HOLD_STEPS = 18;
inline constexpr int BATTLE_WIN_ANIM_TICKS = 10;
inline constexpr int BATTLE_LOSE_ANIM_TICKS = 10;

// Layered battle background parallax (human fighter movement).
inline constexpr int BATTLE_PARALLAX_MAX_LAYERS = 8;
inline constexpr int BATTLE_PARALLAX_LOOP_COPIES = 3;
inline constexpr float BATTLE_PARALLAX_SENSITIVITY = 0.38f;
inline constexpr float BATTLE_PARALLAX_LERP = 0.16f;

// Battle mode round timer (logic steps @ ~60Hz). Tutorial mode has no limit.
inline constexpr int BATTLE_ROUND_TIME_SECONDS = 99;
inline constexpr int BATTLE_ROUND_TIME_STEPS = BATTLE_ROUND_TIME_SECONDS * 60;

// Hit-combo mode: bring the foe to this HP (or below) before time runs out.
inline constexpr int HIT_COMBO_TARGET_HP = 1;
inline constexpr int HIT_COMBO_TIMEOUT_STEPS = 90;
inline constexpr int HIT_COMBO_MIN_DISPLAY = 2;

// Mirror match: tint P2 so same-character fights stay readable.
inline constexpr int MIRROR_MATCH_P2_TINT_R = 255;
inline constexpr int MIRROR_MATCH_P2_TINT_G = 200;
inline constexpr int MIRROR_MATCH_P2_TINT_B = 32;

// Dash attack active frames / box (unscaled units; multiplied by Makoto draw scale).
// Box is anchored ahead of Makoto's center toward facing direction.
inline constexpr int DASH_HIT_START_FRAME = 1;
inline constexpr int DASH_HIT_END_FRAME = 6;
inline constexpr float DASH_HITBOX_WIDTH = 36.0f;
inline constexpr float DASH_HITBOX_HEIGHT = 40.0f;
inline constexpr float DASH_HITBOX_FORWARD = 10.0f;
inline constexpr float DASH_HITBOX_UP = 50.0f;
inline constexpr int DASH_HIT_DAMAGE = 28;

// Legacy compile-time flag. Prefer Fighter::IsHumanControlled() — sandbag is per-instance
// (P2 sandbag by default; any fighter can be sandbag when humanControlled is false).
inline constexpr bool JOKER_SANDBAG_MODE = true;
inline constexpr bool OPPONENT_SANDBAG_MODE = JOKER_SANDBAG_MODE;
// Training heal / no-death. Keep false for real battle SP / damage feedback.
inline constexpr bool TRAINING_MODE = false;
inline constexpr int TRAINING_HEAL_IDLE_FRAMES = 45;
inline constexpr float TUTORIAL_STAMINA_REGEN_MULTIPLIER = 12.0f;

inline constexpr float OPPONENT_MELEE_KNOCKBACK = 6.0f;
inline constexpr float OPPONENT_SKILL_KNOCKBACK = 0.0f;

inline constexpr float JOKER_BODY_HEIGHT = 56.0f;
inline constexpr float JOKER_FEET_Y = 56.0f;
inline constexpr float ARSENE_BODY_HEIGHT = 56.0f;
inline constexpr float ARSENE_FEET_Y = 56.0f;
inline constexpr float ARSENE_BEHIND_HORIZONTAL = 95.0f;
inline constexpr float ARSENE_BEHIND_VERTICAL = 125.0f;
// Mona art sits high in the cell (~y 2-37); feet must match bottom so she stands on ground.
inline constexpr float MONA_TAUNT_OFFSET_X = 100.0f;
inline constexpr float MONA_TAUNT_FEET_Y = 38.0f;
inline constexpr int MONA_TAUNT_FRAME_COUNT = 10;
inline constexpr float AGI_EFFECT_SCALE = 1.0f;
inline constexpr float MABUFU_EFFECT_SCALE = 1.0f;
inline constexpr float THANATOS_SLASH_EFFECT_SCALE = 1.0f;
inline constexpr float PERSONA_EFFECT_SCALE = 4.5f;
// Narukami Izanagi matches Arsene (1.0 body scale), not Makoto's 4.5 personas.
inline constexpr float NARUKAMI_IZANAGI_SCALE = 1.0f;
inline constexpr float NARUKAMI_STANCE_FEET_Y = 52.0f;
inline constexpr float NARUKAMI_RUN_FEET_Y = NARUKAMI_STANCE_FEET_Y;
inline constexpr float NARUKAMI_CROUCH_FEET_Y = 44.0f;
inline constexpr int JOKER_ALL_OUT_TICKS = 7;
inline constexpr int JOKER_ALL_OUT_MEMBER_TICKS = 18;
inline constexpr int JOKER_ALL_OUT_FINISH_TICKS = 14;
inline constexpr int JOKER_ALL_OUT_FINISH_HOLD_FRAMES = 20;
inline constexpr int JOKER_SKILL_TICKS = 5;
inline constexpr int JOKER_TAUNT_TICKS = 7;
// All-out pose lift (pixels up from ground anchor).
inline constexpr float JOKER_ALL_OUT_LIFT_Y = 28.0f;
// Member portraits sit in the top of the cell; high feet keeps the full 3x3 above ground at normal Joker scale.
inline constexpr float JOKER_ALL_OUT_MEMBER_LIFT_Y = 40.0f;
inline constexpr float JOKER_ALL_OUT_MEMBER_FEET_Y = 100.0f;
inline constexpr float JOKER_ALL_OUT_FINISH_LIFT_Y = 0.0f;
// Narukami Zio / Ziodyne body + Izanagi play slower than default summons.
inline constexpr int NARUKAMI_ZIO_SUMMON_TICKS = 10;
inline constexpr int NARUKAMI_ZIO_PERSONA_TICKS = 8;
// Narukami sword-hilt discard flight (behind + up, off-screen).
inline constexpr float NARUKAMI_DISCARD_FLY_X = 14.0f;
inline constexpr float NARUKAMI_DISCARD_FLY_Y = 10.0f;
inline constexpr float NARUKAMI_MYRIAD_IZANAGI_LIFT_Y = 90.0f;
inline constexpr float AGI_MABUFU_OFFSET_X = 14.0f;
inline constexpr float AGI_MABUFU_OFFSET_Y = 8.0f;
inline constexpr int ACTION_VISUAL_HOLD_FRAMES = 0;
inline constexpr int INTRO_TO_STANCE_BLEND_FRAMES = 0;
inline constexpr int INTRO_VISUAL_HOLD_FRAMES = 8;
inline constexpr int PERSONA_ANIM_DELAY = 5;
inline constexpr int PERSONA_EFFECT_ANIM_DELAY = 3;
// Laser-type supers (Maziodyne / Ziodyne): slower sheet + hold last bolt for impact.
inline constexpr int LASER_EFFECT_ANIM_DELAY = 9;
inline constexpr int LASER_END_HOLD_FRAMES = 18;
// Makoto Thanatos + Maziodyne: faster bolt cycle than generic laser supers.
inline constexpr int MAZIODYNE_LASER_EFFECT_ANIM_DELAY = 5;
inline constexpr int MAZIODYNE_LASER_END_HOLD_FRAMES = 8;
inline constexpr int NARUKAMI_LASER_PERSONA_TICKS = 14;
inline constexpr int NARUKAMI_LASER_END_HOLD_STEPS = 24;
// Narukami Zio/Ziodyne: Izanagi in front of Yu (normal scale) + bolt like Thanatos/Maziodyne.
inline constexpr float NARUKAMI_IZANAGI_SPRAY_FORWARD = 108.0f;
inline constexpr float NARUKAMI_IZANAGI_SPRAY_VERTICAL = 95.0f;
inline constexpr float NARUKAMI_ZIO_HIT_BODY_Y_RATIO = 0.78f;
// Ziodyne beam anchor: pulled back from foe toward Yu (same idea as MAZIODYNE_*).
inline constexpr float NARUKAMI_ZIODYNE_HORIZONTAL_OFFSET = 50.0f;
// Ziodyne art sits high in the cell — push the feet anchor down to torso center.
inline constexpr float NARUKAMI_ZIODYNE_VERTICAL_OFFSET = 52.0f;
inline constexpr float ULTIMATE_PULL_LERP = 0.42f;
inline constexpr float ULTIMATE_PULL_SCREEN_MARGIN = 48.0f;
inline constexpr int THANATOS_SLASH_ANIM_DELAY = 4;
inline constexpr int PERSONA_STANCE_ANIM_TICKS = 6;
inline constexpr int MAKOTO_INTRO_TICKS = 10;
// Hold last intro frame briefly before stance so the handoff does not flash.
inline constexpr int INTRO_END_HOLD_FRAMES = 8;
inline constexpr int MAKOTO_SUMMON_AIR_TICKS = 6;
inline constexpr int IDLE_THRESHOLD_FRAMES = 600;
inline constexpr int MEGIDOLAON_BURST_FRAME_COUNT = 8;
inline constexpr int MESSIAH_SUMMON_FRAME_COUNT = 18;
inline constexpr int MESSIAH_EFFECT_FRAME_COUNT = 8;
inline constexpr int MEGIDOLAON_EFFECT_FRAME_COUNT = 16;
inline constexpr float MEGIDOLAON_BURST_SCALE_FACTOR = 3.0f;
inline constexpr float EFFECT_ANCHOR_CENTER = 0.5f;
inline constexpr float EFFECT_ANCHOR_BOTTOM = 1.0f;
inline constexpr BYTE HUD_HP_R = 156;
inline constexpr BYTE HUD_HP_G = 247;
inline constexpr BYTE HUD_HP_B = 240;
inline constexpr BYTE HUD_SP_R = 253;
inline constexpr BYTE HUD_SP_G = 251;
inline constexpr BYTE HUD_SP_B = 140;
inline constexpr BYTE HUD_HP_DELAYED_R = 255;
inline constexpr BYTE HUD_HP_DELAYED_G = 255;
inline constexpr BYTE HUD_HP_DELAYED_B = 255;

inline constexpr int SP_COST_SUMMON_1 = 20;
inline constexpr int SP_COST_SUMMON_2 = 20;
inline constexpr int SP_COST_SUMMON_AIR = 35;
inline constexpr int SP_COST_SUMMON_AIR_2 = 45;
inline constexpr int SP_COST_THANATOS_SLASH = 25;
inline constexpr int SP_GAIN_ON_HIT = 15;

// Stamina: 3 bars under SP. Most special mobility / heavy moves cost 1 full bar.
inline constexpr float FIGHTER_MAX_STAMINA = 3.0f;
inline constexpr float STAMINA_COST_ACTION = 1.0f;
// Gradual run drain per animation step (not a full bar). Tuned so a long sprint empties ~1 bar quickly.
inline constexpr float STAMINA_RUN_DRAIN_PER_STEP = 0.012f;
// Slow passive refill while not sprinting (~1 bar every few seconds).
inline constexpr float STAMINA_REGEN_PER_STEP = 0.005f;

inline constexpr BYTE HUD_STAMINA_R = 120;
inline constexpr BYTE HUD_STAMINA_G = 220;
inline constexpr BYTE HUD_STAMINA_B = 90;
inline constexpr float HUD_STAMINA_BAR_HEIGHT = 8.0f;
inline constexpr float HUD_STAMINA_SEGMENT_GAP = 4.0f;

inline constexpr int MAKOTO_MAX_HEALTH = 400;
inline constexpr int NARUKAMI_MAX_HEALTH = 400;
inline constexpr int JOKER_MAX_HEALTH = 400;
inline constexpr int FIGHTER_MAX_SP = 100;

inline constexpr float HUD_EDGE_MARGIN = 40.0f;
inline constexpr float HUD_TOP_Y = 18.0f;
inline constexpr float HUD_ICON_SIZE = 56.0f;
inline constexpr float HUD_ICON_BAR_GAP = 12.0f;
inline constexpr float HUD_BAR_WIDTH = 240.0f;
inline constexpr float HUD_HP_BAR_HEIGHT = 16.0f;
inline constexpr float HUD_SP_BAR_HEIGHT = 10.0f;
inline constexpr float HUD_BAR_GAP = 5.0f;
inline constexpr float HUD_NAME_OFFSET_Y = -22.0f;
inline constexpr float HUD_COMBO_OFFSET_Y = 8.0f;

inline constexpr int JOKER_WIN_RUN_SPEED = 10;
inline constexpr float JOKER_WIN_RUN_OFFSCREEN_MARGIN = 120.0f;
inline constexpr float PERSONA_SUMMON_SFX_VOLUME = 2.0f;
// Font assets (AddFontResourceEx path + D3DXCreateFont family name)
inline constexpr const char* NORMAL_FONT_FILE = "assets/font/normal_font.TTF";
inline constexpr const char* NORMAL_FONT_FAMILY = "BM space";
inline constexpr const char* MAINMENU_FONT_FILE = "assets/font/mainmenu_font.ttf";
inline constexpr const char* MAINMENU_FONT_FAMILY = "Persona 5 Menu Font Prototype";
inline constexpr const char* GAMETITLE_FONT_FILE = "assets/font/gametitle_font.ttf";
inline constexpr const char* GAMETITLE_FONT_FAMILY = "Markin LT UltraBold";

// HUD / player name uses normal_font
inline constexpr const char* HUD_FONT_FILE = NORMAL_FONT_FILE;
inline constexpr const char* HUD_FONT_FILE_ALT = NORMAL_FONT_FILE;
inline constexpr const char* HUD_FONT_FAMILY = NORMAL_FONT_FAMILY;
inline constexpr int MAKOTO_CELL_SIZE = 256;
inline constexpr int NARUKAMI_CELL_SIZE = 256;
inline constexpr int GAME_ANIMATION_FPS = 60;
inline constexpr int MAKOTO_LOOP_TICKS_SLOW = 4;
inline constexpr int MAKOTO_LOOP_TICKS_FAST = 3;
inline constexpr int MAKOTO_IDLE_PLAY_TICKS = 6;
inline constexpr int MAKOTO_ACTION_TICKS = 3;
inline constexpr int MAKOTO_SUMMON_TICKS = 4;
inline constexpr int MAKOTO_IDLE_TICKS = 15;

struct SpriteSheetBounds {
    float maxFrameWidth;
    float maxFrameHeight;
};

inline SpriteSheetBounds MeasureTextureRange(LPDIRECT3DTEXTURE9* textures, int startIndex, int endIndex) {
    SpriteSheetBounds bounds = { 0.0f, 0.0f };
    for (int textureIndex = startIndex; textureIndex < endIndex; ++textureIndex) {
        if (textures[textureIndex] == NULL) {
            continue;
        }
        D3DSURFACE_DESC surfaceDescription;
        textures[textureIndex]->GetLevelDesc(0, &surfaceDescription);
        if ((float)surfaceDescription.Width > bounds.maxFrameWidth) {
            bounds.maxFrameWidth = (float)surfaceDescription.Width;
        }
        if ((float)surfaceDescription.Height > bounds.maxFrameHeight) {
            bounds.maxFrameHeight = (float)surfaceDescription.Height;
        }
    }
    return bounds;
}

extern SpriteSheetBounds g_MessiahSheetBounds;
extern SpriteSheetBounds g_MegidolaonBurstBounds;
extern SpriteSheetBounds g_MegidolaonBlastBounds;

inline float GetCharacterRenderScale() {
    return ((float)SCREEN_HEIGHT * CHARACTER_SCREEN_HEIGHT_RATIO) / CHARACTER_REFERENCE_HEIGHT;
}

inline float GetMakotoDrawScale() {
    return ((float)SCREEN_HEIGHT * MAKOTO_SCREEN_HEIGHT_RATIO) / MAKOTO_BODY_HEIGHT;
}

inline float GetJokerDrawScale() {
    return ((float)SCREEN_HEIGHT * MAKOTO_SCREEN_HEIGHT_RATIO) / JOKER_BODY_HEIGHT;
}

inline float GetPersonaEffectDrawScale() {
    return ((float)SCREEN_HEIGHT * MAKOTO_SCREEN_HEIGHT_RATIO) / (float)MAKOTO_CELL_SIZE;
}

inline float GetMakotoScreenHalfWidth() {
    return MAKOTO_BODY_WIDTH * 0.5f * GetMakotoDrawScale();
}

// Default P1↔P2 center distance at round start (used by ultimate pull-in).
inline float GetDefaultBattleCenterGap() {
    const float p1SpawnX = GetMakotoScreenHalfWidth() + MAKOTO_WINDOW_MARGIN + MAKOTO_SPAWN_FORWARD;
    return OPPONENT_SPAWN_X - p1SpawnX;
}

inline float GetJokerScreenHalfWidth() {
    return JOKER_PUSHBOX_WIDTH * 0.5f * GetJokerDrawScale();
}

inline void ClampFighterCenterX(float& centerX, float bodyHalfWidth) {
    float minX = bodyHalfWidth + MAKOTO_WINDOW_MARGIN;
    float maxX = (float)SCREEN_WIDTH - minX;
    if (centerX < minX) centerX = minX;
    if (centerX > maxX) centerX = maxX;
}

inline void ClampMakotoCenterX(float& centerX) {
    ClampFighterCenterX(centerX, GetMakotoScreenHalfWidth());
}

inline void ClampJokerCenterX(float& centerX) {
    ClampFighterCenterX(centerX, GetJokerScreenHalfWidth());
}

// Live pushbox from current feet anchor — matches DrawScaledCharacterSprite layout.
inline AABB MakeLivePushbox(
    const D3DXVECTOR3& pos,
    int facingDirection,
    float bodyWidth,
    float bodyHeight,
    float drawScale)
{
    const float w = bodyWidth * drawScale;
    const float h = bodyHeight * drawScale;
    const float anchor = MAKOTO_BODY_CENTER_X * drawScale;
    AABB box;
    box.width = w;
    box.height = h;
    box.y = pos.y - h;
    if (facingDirection < 0) {
        box.x = pos.x + anchor - w;
    }
    else {
        box.x = pos.x - anchor;
    }
    return box;
}

inline float GetEffectRenderScale() {
    return ((float)SCREEN_HEIGHT * 0.18f) / CHARACTER_REFERENCE_HEIGHT;
}

inline float GetMessiahRenderScale() {
    return ((float)SCREEN_HEIGHT * 0.26f) / MESSIAH_REFERENCE_HEIGHT;
}

inline float GetMegidolaonRenderScale() {
    return ((float)SCREEN_HEIGHT * 0.16f) / MEGIDOLAON_REFERENCE_HEIGHT;
}

inline constexpr BYTE PERSONA_COLORKEY_R = 7;
inline constexpr BYTE PERSONA_COLORKEY_G = 115;
inline constexpr BYTE PERSONA_COLORKEY_B = 255;

inline constexpr BYTE JOKER_COLORKEY_R = 232;
inline constexpr BYTE JOKER_COLORKEY_G = 4;
inline constexpr BYTE JOKER_COLORKEY_B = 4;

// Yosuke / Jiraiya sheet background key (warm yellow).
inline constexpr BYTE YOSUKE_COLORKEY_R = 255;
inline constexpr BYTE YOSUKE_COLORKEY_G = 200;
inline constexpr BYTE YOSUKE_COLORKEY_B = 84;

inline constexpr int YOSUKE_MAX_HEALTH = 400;
inline constexpr int YOSUKE_MOVE_SPEED = 6;
inline constexpr float YOSUKE_STANCE_FEET_Y = 52.0f;
inline constexpr float YOSUKE_RUN_FEET_Y = YOSUKE_STANCE_FEET_Y;
inline constexpr float FIGHTER_RUN_BLEND_RATE = 0.14f;
inline constexpr float FIGHTER_RUN_ANIM_THRESHOLD = 0.55f;
inline constexpr float YOSUKE_JIRAIYA_SCALE = 1.0f;
inline constexpr float YOSUKE_INTRO_DROP_START_Y = -120.0f;

// Narukami sheet background key (yellow).
inline constexpr BYTE NARUKAMI_COLORKEY_R = 249;
inline constexpr BYTE NARUKAMI_COLORKEY_G = 254;
inline constexpr BYTE NARUKAMI_COLORKEY_B = 56;

// Tekken-style HP chip: hold after hit, then drain only while no new damage.
inline constexpr int HUD_HP_CHIP_HOLD_FRAMES = 50;
// Full chip drains over this many frames once hold ends (~1.5s at 60fps).
inline constexpr int HUD_HP_CHIP_DRAIN_FRAMES = 90;

inline void ApplyTextureColorKey(LPDIRECT3DTEXTURE9 tex, BYTE keyR, BYTE keyG, BYTE keyB) {
    if (!tex) return;

    D3DSURFACE_DESC desc;
    tex->GetLevelDesc(0, &desc);

    D3DLOCKED_RECT rect;
    if (FAILED(tex->LockRect(0, &rect, NULL, 0))) return;

    for (UINT y = 0; y < desc.Height; y++) {
        DWORD* row = (DWORD*)((BYTE*)rect.pBits + y * rect.Pitch);
        for (UINT x = 0; x < desc.Width; x++) {
            DWORD pixel = row[x];
            BYTE r = (pixel >> 16) & 0xFF;
            BYTE g = (pixel >> 8) & 0xFF;
            BYTE b = pixel & 0xFF;
            if (r == keyR && g == keyG && b == keyB) {
                row[x] = 0x00000000;
            }
        }
    }

    tex->UnlockRect(0);
}

inline void ApplyJokerColorKey(LPDIRECT3DTEXTURE9 tex) {
    ApplyTextureColorKey(tex, JOKER_COLORKEY_R, JOKER_COLORKEY_G, JOKER_COLORKEY_B);
}

inline void ApplyPersonaBlueColorKey(LPDIRECT3DTEXTURE9 tex) {
    ApplyTextureColorKey(tex, PERSONA_COLORKEY_R, PERSONA_COLORKEY_G, PERSONA_COLORKEY_B);
}

inline void ApplyYosukeColorKey(LPDIRECT3DTEXTURE9 tex) {
    ApplyTextureColorKey(tex, YOSUKE_COLORKEY_R, YOSUKE_COLORKEY_G, YOSUKE_COLORKEY_B);
}

inline void ApplyNarukamiColorKey(LPDIRECT3DTEXTURE9 tex) {
    // Exact key plus a small tolerance for compression variants of the yellow BG.
    if (!tex) return;

    D3DSURFACE_DESC desc;
    tex->GetLevelDesc(0, &desc);

    D3DLOCKED_RECT rect;
    if (FAILED(tex->LockRect(0, &rect, NULL, 0))) return;

    constexpr int kTolerance = 8;
    for (UINT y = 0; y < desc.Height; y++) {
        DWORD* row = (DWORD*)((BYTE*)rect.pBits + y * rect.Pitch);
        for (UINT x = 0; x < desc.Width; x++) {
            DWORD pixel = row[x];
            const int r = (int)((pixel >> 16) & 0xFF);
            const int g = (int)((pixel >> 8) & 0xFF);
            const int b = (int)(pixel & 0xFF);
            if (abs(r - (int)NARUKAMI_COLORKEY_R) <= kTolerance &&
                abs(g - (int)NARUKAMI_COLORKEY_G) <= kTolerance &&
                abs(b - (int)NARUKAMI_COLORKEY_B) <= kTolerance) {
                row[x] = 0x00000000;
            }
        }
    }

    tex->UnlockRect(0);
}

inline void DrawScaledCharacterSprite(
    LPD3DXSPRITE sprite,
    LPDIRECT3DTEXTURE9 tex,
    const RECT* srcRect,
    const D3DXVECTOR3& pos,
    int facingDirection,
    float scale,
    D3DCOLOR color,
    float sourceContentHeight = 0.0f,
    float sourceFeetY = 0.0f)
{
    if (!sprite || !tex) return;

    int srcW = MAKOTO_CELL_SIZE;
    int srcH = MAKOTO_CELL_SIZE;
    if (srcRect) {
        srcW = srcRect->right - srcRect->left;
        srcH = srcRect->bottom - srcRect->top;
    }
    else {
        D3DSURFACE_DESC desc;
        tex->GetLevelDesc(0, &desc);
        srcW = (int)desc.Width;
        srcH = (int)desc.Height;
    }

    float screenHeightRatio = (sourceContentHeight > 0.0f)
        ? MAKOTO_SCREEN_HEIGHT_RATIO
        : CHARACTER_SCREEN_HEIGHT_RATIO;
    float targetScreenHeight = (float)SCREEN_HEIGHT * screenHeightRatio;
    float contentHeight = (sourceContentHeight > 0.0f) ? sourceContentHeight : (float)srcH;
    float drawScale = targetScreenHeight / contentHeight;
    if (sourceContentHeight > 0.0f && scale > 0.0f) {
        drawScale *= scale;
    }
    float drawW = (float)srcW * drawScale;
    float drawH = (float)srcH * drawScale;
    float feetY = (sourceFeetY > 0.0f) ? sourceFeetY : (float)srcH;
    float anchorX = (sourceContentHeight > 0.0f) ? MAKOTO_BODY_CENTER_X : (drawW * 0.5f);
    float offsetY = pos.y - feetY * drawScale;
    float offsetX = (facingDirection == -1) ? pos.x + anchorX * drawScale : pos.x - anchorX * drawScale;

    D3DXMATRIX matFlip, matScale, matTrans, matFinal;
    if (facingDirection == -1) {
        D3DXMatrixScaling(&matFlip, -1.0f, 1.0f, 1.0f);
    }
    else {
        D3DXMatrixIdentity(&matFlip);
    }
    D3DXMatrixScaling(&matScale, drawScale, drawScale, 1.0f);
    D3DXMatrixTranslation(&matTrans, offsetX, offsetY, pos.z);
    matFinal = matFlip * matScale * matTrans;
    sprite->SetTransform(&matFinal);

    D3DXVECTOR3 zeroPos(0.0f, 0.0f, 0.0f);
    sprite->Draw(tex, srcRect, NULL, &zeroPos, color);

    D3DXMATRIX matIdentity;
    D3DXMatrixIdentity(&matIdentity);
    sprite->SetTransform(&matIdentity);
}

inline void DrawCenteredEffectSprite(
    LPD3DXSPRITE sprite,
    LPDIRECT3DTEXTURE9 tex,
    const RECT* srcRect,
    const D3DXVECTOR3& centerPos,
    float scale,
    D3DCOLOR color,
    float sourceContentHeight = 0.0f)
{
    if (!sprite || !tex) return;

    int srcW = 0;
    int srcH = 0;
    if (srcRect) {
        srcW = srcRect->right - srcRect->left;
        srcH = srcRect->bottom - srcRect->top;
    }
    else {
        D3DSURFACE_DESC desc;
        tex->GetLevelDesc(0, &desc);
        srcW = (int)desc.Width;
        srcH = (int)desc.Height;
    }

    float screenHeightRatio = (sourceContentHeight > 0.0f)
        ? MAKOTO_SCREEN_HEIGHT_RATIO
        : CHARACTER_SCREEN_HEIGHT_RATIO;
    float targetScreenHeight = (float)SCREEN_HEIGHT * screenHeightRatio;
    float contentHeight = (sourceContentHeight > 0.0f) ? sourceContentHeight : (float)srcH;
    float drawScale = targetScreenHeight / contentHeight;
    if (scale > 0.0f) {
        drawScale *= scale;
    }
    float drawW = (float)srcW * drawScale;
    float drawH = (float)srcH * drawScale;
    float offsetX = centerPos.x - drawW * 0.5f;
    float offsetY = centerPos.y - drawH * 0.5f;

    D3DXMATRIX matScale, matTrans, matFinal;
    D3DXMatrixScaling(&matScale, drawScale, drawScale, 1.0f);
    D3DXMatrixTranslation(&matTrans, offsetX, offsetY, centerPos.z);
    matFinal = matScale * matTrans;
    sprite->SetTransform(&matFinal);

    D3DXVECTOR3 zeroPos(0.0f, 0.0f, 0.0f);
    sprite->Draw(tex, srcRect, NULL, &zeroPos, color);

    D3DXMATRIX matIdentity;
    D3DXMatrixIdentity(&matIdentity);
    sprite->SetTransform(&matIdentity);
}

inline void DrawAnchoredEffectSprite(
    LPD3DXSPRITE sprite,
    LPDIRECT3DTEXTURE9 tex,
    const D3DXVECTOR3& anchorPos,
    float scale,
    D3DCOLOR color,
    float refWidth,
    float refHeight,
    float anchorNormX,
    float anchorNormY)
{
    if (!sprite || !tex) return;

    D3DSURFACE_DESC desc;
    tex->GetLevelDesc(0, &desc);
    float drawW = (float)desc.Width * scale;
    float drawH = (float)desc.Height * scale;
    float refDrawW = refWidth * scale;
    float refDrawH = refHeight * scale;
    float offsetX = anchorPos.x - refDrawW * anchorNormX + (refDrawW - drawW) * 0.5f;
    float offsetY = anchorPos.y - refDrawH * anchorNormY + (refDrawH - drawH) * 0.5f;

    D3DXMATRIX matScale, matTrans, matFinal;
    D3DXMatrixScaling(&matScale, scale, scale, 1.0f);
    D3DXMatrixTranslation(&matTrans, offsetX, offsetY, anchorPos.z);
    matFinal = matScale * matTrans;
    sprite->SetTransform(&matFinal);

    D3DXVECTOR3 zeroPos(0.0f, 0.0f, 0.0f);
    sprite->Draw(tex, NULL, NULL, &zeroPos, color);

    D3DXMATRIX matIdentity;
    D3DXMatrixIdentity(&matIdentity);
    sprite->SetTransform(&matIdentity);
}

// ============ Direct3D ============
extern IDirect3D9* g_pD3D;
extern IDirect3DDevice9* g_pD3DDevice;
extern D3DPRESENT_PARAMETERS d3dpp;
extern LPD3DXSPRITE spriteBrush;

// ============ DirectInput ============
extern LPDIRECTINPUT8 dInput;
extern LPDIRECTINPUTDEVICE8 dInputKeyboardDevice;
extern BYTE diKeys[256];

// Melee attack description.
// Frames are animation indices where the hitbox is active.
// offset/size are unscaled body units relative to fighter pose (scaled at runtime).
struct AttackData {
    int startFrame;
    int endFrame;
    int damage;
    float offsetX;
    float offsetY;
    float width;
    float height;
};

// Neutral / crouch / neutral-air attack (jab).
inline constexpr int ATK_NEUTRAL_START = 3;
inline constexpr int ATK_NEUTRAL_END = 10;
inline constexpr int ATK_NEUTRAL_DAMAGE = 28;
inline constexpr float ATK_NEUTRAL_OFFSET_X = 18.0f;
inline constexpr float ATK_NEUTRAL_OFFSET_Y = -18.0f;
inline constexpr float ATK_NEUTRAL_WIDTH = 28.0f;
inline constexpr float ATK_NEUTRAL_HEIGHT = 40.0f;

// Side attack / side-air.
inline constexpr int ATK_SIDE_START = 2;
inline constexpr int ATK_SIDE_END = 8;
inline constexpr int ATK_SIDE_DAMAGE = 32;
inline constexpr float ATK_SIDE_OFFSET_X = 28.0f;
inline constexpr float ATK_SIDE_OFFSET_Y = -22.0f;
inline constexpr float ATK_SIDE_WIDTH = 36.0f;
inline constexpr float ATK_SIDE_HEIGHT = 42.0f;

// Up attack / up-air.
inline constexpr int ATK_UP_START = 2;
inline constexpr int ATK_UP_END = 5;
inline constexpr int ATK_UP_DAMAGE = 32;
inline constexpr float ATK_UP_OFFSET_X = 22.0f;
inline constexpr float ATK_UP_OFFSET_Y = -22.0f;
inline constexpr float ATK_UP_WIDTH = 32.0f;
inline constexpr float ATK_UP_HEIGHT = 48.0f;

// Down attack / down-air.
inline constexpr int ATK_DOWN_START = 2;
inline constexpr int ATK_DOWN_END = 8;
inline constexpr int ATK_DOWN_DAMAGE = 30;
inline constexpr float ATK_DOWN_OFFSET_X = 14.0f;
inline constexpr float ATK_DOWN_OFFSET_Y = -36.0f;
inline constexpr float ATK_DOWN_WIDTH = 40.0f;
inline constexpr float ATK_DOWN_HEIGHT = 32.0f;

void DrawDebugRect(LPD3DXSPRITE sprite, float x, float y, float w, float h, D3DCOLOR color);
void DrawDebugCircleRing(LPD3DXSPRITE sprite, float cx, float cy, float radius, D3DCOLOR color, int segments = 48);

// Shared melee AttackData instances (defined in main.cpp).
extern AttackData attackHitbox;
extern AttackData sideAttackHitbox;
extern AttackData attackUpHitbox;
extern AttackData downAttackHitbox;

// Battle background texture (aliased from the selected stage).
extern LPDIRECT3DTEXTURE9 texBgCity1;