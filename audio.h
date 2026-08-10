#pragma once

// Sound module (BMCS2224) - FMOD wrapper.
// Owns the FMOD system, loops menu/battle music, and plays one-shot UI SFX.

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
    void* musicChannel;
    void* currentTrack;
    bool isInitialised;
    bool isPlaying;
    bool isMuted;
};

extern SoundManager g_SoundManager;
