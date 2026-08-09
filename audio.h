#pragma once

// Sound module (BMCS2224) - FMOD wrapper
class SoundManager {
public:
    SoundManager();
    ~SoundManager();

    bool Initialise();
    void PlayBattleMusic();
    void StopBattleMusic();
    void ToggleMusicMute();
    bool IsMusicMuted() const { return isMuted; }
    void Update();
    void Shutdown();

private:
    void* fmodSystem;
    void* battleMusic;
    void* musicChannel;
    bool isInitialised;
    bool isPlaying;
    bool isMuted;
};

extern SoundManager g_SoundManager;
