#include "fmod.hpp"
#include "fmod_errors.h"
#include "audio.h"

SoundManager g_SoundManager;

SoundManager::SoundManager()
    : fmodSystem(nullptr), battleMusic(nullptr), musicChannel(nullptr),
      isInitialised(false), isPlaying(false), isMuted(false) {
}

SoundManager::~SoundManager() {
    Shutdown();
}

bool SoundManager::Initialise() {
    if (isInitialised) {
        return true;
    }

    FMOD::System* system = nullptr;
    FMOD_RESULT result = FMOD::System_Create(&system);
    if (result != FMOD_OK) {
        return false;
    }

    result = system->init(32, FMOD_INIT_NORMAL, nullptr);
    if (result != FMOD_OK) {
        system->release();
        return false;
    }

    FMOD::Sound* track = nullptr;
    result = system->createSound(
        "assets/sound/battle_music.mp3",
        FMOD_LOOP_NORMAL | FMOD_DEFAULT,
        nullptr,
        &track);

    if (result != FMOD_OK) {
        system->close();
        system->release();
        return false;
    }

    fmodSystem = system;
    battleMusic = track;
    isInitialised = true;
    return true;
}

void SoundManager::PlayBattleMusic() {
    if (!isInitialised || isPlaying) {
        return;
    }

    FMOD::System* system = static_cast<FMOD::System*>(fmodSystem);
    FMOD::Sound* track = static_cast<FMOD::Sound*>(battleMusic);
    FMOD::Channel* channel = nullptr;

    if (system->playSound(track, nullptr, false, &channel) == FMOD_OK) {
        musicChannel = channel;
        isPlaying = true;
        if (isMuted) {
            channel->setMute(true);
        }
    }
}

void SoundManager::ToggleMusicMute() {
    isMuted = !isMuted;
    if (musicChannel != nullptr) {
        FMOD::Channel* channel = static_cast<FMOD::Channel*>(musicChannel);
        channel->setMute(isMuted);
    }
}

void SoundManager::StopBattleMusic() {
    if (!isPlaying || musicChannel == nullptr) {
        return;
    }

    FMOD::Channel* channel = static_cast<FMOD::Channel*>(musicChannel);
    channel->stop();
    musicChannel = nullptr;
    isPlaying = false;
}

void SoundManager::Update() {
    if (!isInitialised) {
        return;
    }

    FMOD::System* system = static_cast<FMOD::System*>(fmodSystem);
    system->update();
}

void SoundManager::Shutdown() {
    StopBattleMusic();

    if (battleMusic != nullptr) {
        static_cast<FMOD::Sound*>(battleMusic)->release();
        battleMusic = nullptr;
    }

    if (fmodSystem != nullptr) {
        FMOD::System* system = static_cast<FMOD::System*>(fmodSystem);
        system->close();
        system->release();
        fmodSystem = nullptr;
    }

    isInitialised = false;
}
