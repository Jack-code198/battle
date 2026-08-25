#pragma once

#include "config.h"

// =============================================================================
// Sound module (BMCS2224) — OO FMOD wrapper.
// Initialise / Update / Shutdown own the FMOD system. Menu + battle BGM loop;
// UI selection is a one-shot. Mutes via ToggleMusicMute without tearing down FMOD.
// =============================================================================

class SoundManager {
public:
    SoundManager();
    ~SoundManager();

    bool Initialise();
    void PlayMenuMusic();
    void StopMenuMusic();
    void PlayBattleMusic();
    void StopBattleMusic();
    void PlaySelectionSound();
    void PlayPersonaSummonSfx(float volume = PERSONA_SUMMON_SFX_VOLUME);
    void ToggleMusicMute();
    bool IsMusicMuted() const { return isMuted; }
    void Update();
    void Shutdown();

private:
    void StopCurrentMusic();
    void PlayMusicTrack(void* track);

    void* fmodSystem;
    void* menuMusic;
    void* battleMusic;
    void* selectionSound;
    void* personaSummonSfx;
    void* musicChannel;
    void* currentTrack;
    bool isInitialised;
    bool isPlaying;
    bool isMuted;
};

extern SoundManager g_SoundManager;
