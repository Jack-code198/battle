#include "fmod.hpp"
#include "fmod_errors.h"
#include "audio.h"

SoundManager g_SoundManager;

SoundManager::SoundManager()
    : fmodSystem(nullptr), menuMusic(nullptr), battleMusic(nullptr), creditsMusic(nullptr),
      selectionSound(nullptr),
      personaSummonSfx(nullptr), musicChannel(nullptr), currentTrack(nullptr),
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

    FMOD::Sound* menuTrack = nullptr;
    result = system->createSound(
        "assets/sound/mainmenu_music.mp3",
        FMOD_LOOP_NORMAL | FMOD_DEFAULT,
        nullptr,
        &menuTrack);
    if (result != FMOD_OK) {
        menuTrack = nullptr;
    }

    FMOD::Sound* battleTrack = nullptr;
    result = system->createSound(
        "assets/sound/battle_music.mp3",
        FMOD_LOOP_NORMAL | FMOD_DEFAULT,
        nullptr,
        &battleTrack);
    if (result != FMOD_OK) {
        battleTrack = nullptr;
    }

    FMOD::Sound* creditsTrack = nullptr;
    result = system->createSound(
        "assets/sound/end_credits_song.mp3",
        FMOD_LOOP_NORMAL | FMOD_DEFAULT,
        nullptr,
        &creditsTrack);
    if (result != FMOD_OK) {
        creditsTrack = nullptr;
    }

    FMOD::Sound* selectSfx = nullptr;
    result = system->createSound(
        "assets/sound/selection_sound.mp3",
        FMOD_DEFAULT,
        nullptr,
        &selectSfx);
    if (result != FMOD_OK) {
        selectSfx = nullptr;
    }

    FMOD::Sound* personaSummon = nullptr;
    result = system->createSound(
        "assets/sound/persona_summon_sound_effect.mp3",
        FMOD_DEFAULT,
        nullptr,
        &personaSummon);
    if (result != FMOD_OK) {
        personaSummon = nullptr;
    }

    if (!menuTrack && !battleTrack) {
        if (selectSfx) {
            selectSfx->release();
        }
        if (personaSummon) {
            personaSummon->release();
        }
        system->close();
        system->release();
        return false;
    }

    fmodSystem = system;
    menuMusic = menuTrack;
    battleMusic = battleTrack;
    creditsMusic = creditsTrack;
    selectionSound = selectSfx;
    personaSummonSfx = personaSummon;
    isInitialised = true;
    return true;
}

void SoundManager::StopCurrentMusic() {
    if (!isPlaying || musicChannel == nullptr) {
        return;
    }

    FMOD::Channel* channel = static_cast<FMOD::Channel*>(musicChannel);
    channel->stop();
    musicChannel = nullptr;
    currentTrack = nullptr;
    isPlaying = false;
}

void SoundManager::PlayMusicTrack(void* track) {
    if (!isInitialised || track == nullptr) {
        return;
    }

    if (isPlaying && currentTrack == track) {
        return;
    }

    StopCurrentMusic();

    FMOD::System* system = static_cast<FMOD::System*>(fmodSystem);
    FMOD::Sound* sound = static_cast<FMOD::Sound*>(track);
    FMOD::Channel* channel = nullptr;

    if (system->playSound(sound, nullptr, false, &channel) == FMOD_OK) {
        musicChannel = channel;
        currentTrack = track;
        isPlaying = true;
        if (isMuted) {
            channel->setMute(true);
        }
    }
}

void SoundManager::PlayMenuMusic() {
    PlayMusicTrack(menuMusic);
}

void SoundManager::StopMenuMusic() {
    if (currentTrack == menuMusic) {
        StopCurrentMusic();
    }
}

void SoundManager::PlayBattleMusic() {
    PlayMusicTrack(battleMusic);
}

void SoundManager::StopBattleMusic() {
    if (currentTrack == battleMusic) {
        StopCurrentMusic();
    }
}

void SoundManager::PlayCreditsMusic() {
    PlayMusicTrack(creditsMusic);
}

void SoundManager::StopCreditsMusic() {
    if (currentTrack == creditsMusic) {
        StopCurrentMusic();
    }
}

void SoundManager::PlaySelectionSound() {
    if (!isInitialised || selectionSound == nullptr) {
        return;
    }

    FMOD::System* system = static_cast<FMOD::System*>(fmodSystem);
    FMOD::Sound* sound = static_cast<FMOD::Sound*>(selectionSound);
    FMOD::Channel* channel = nullptr;

    if (system->playSound(sound, nullptr, false, &channel) == FMOD_OK && channel != nullptr) {
        if (isMuted) {
            channel->setMute(true);
        }
    }
}

void SoundManager::PlayPersonaSummonSfx(float volume) {
    if (!isInitialised || personaSummonSfx == nullptr) {
        return;
    }

    FMOD::System* system = static_cast<FMOD::System*>(fmodSystem);
    FMOD::Sound* sound = static_cast<FMOD::Sound*>(personaSummonSfx);
    FMOD::Channel* channel = nullptr;

    if (system->playSound(sound, nullptr, false, &channel) == FMOD_OK && channel != nullptr) {
        channel->setVolume(volume);
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

void SoundManager::Update() {
    if (!isInitialised) {
        return;
    }

    FMOD::System* system = static_cast<FMOD::System*>(fmodSystem);
    system->update();
}

void SoundManager::Shutdown() {
    StopCurrentMusic();

    if (menuMusic != nullptr) {
        static_cast<FMOD::Sound*>(menuMusic)->release();
        menuMusic = nullptr;
    }

    if (battleMusic != nullptr) {
        static_cast<FMOD::Sound*>(battleMusic)->release();
        battleMusic = nullptr;
    }

    if (creditsMusic != nullptr) {
        static_cast<FMOD::Sound*>(creditsMusic)->release();
        creditsMusic = nullptr;
    }

    if (selectionSound != nullptr) {
        static_cast<FMOD::Sound*>(selectionSound)->release();
        selectionSound = nullptr;
    }

    if (personaSummonSfx != nullptr) {
        static_cast<FMOD::Sound*>(personaSummonSfx)->release();
        personaSummonSfx = nullptr;
    }

    if (fmodSystem != nullptr) {
        FMOD::System* system = static_cast<FMOD::System*>(fmodSystem);
        system->close();
        system->release();
        fmodSystem = nullptr;
    }

    isInitialised = false;
}
