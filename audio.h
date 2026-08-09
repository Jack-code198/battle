#pragma once

// Sound module (BMCS2224) - FMOD wrapper
class SoundManager {
public:
    SoundManager();
    ~SoundManager();

    bool Initialise();
    void PlayMenuMusic();
    void StopMenuMusic();
    void PlayBattleMusic();
    void StopBattleMusic();
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
    void* musicChannel;
    void* currentTrack;
    bool isInitialised;
    bool isPlaying;
    bool isMuted;
};

extern SoundManager g_SoundManager;
