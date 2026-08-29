#include "fmod.hpp"
#include "fmod_errors.h"
#include "audio.h"

// =============================================================================
// Sound module (BMCS2224) — FMOD lifecycle.
// Initialise: System_Create -> init -> createSound (menu/battle/credits/SFX).
// Update:     call system->update() once per frame from the main loop.
// Shutdown:   stop channels, release sounds, close and release the system.
// Battle BGM (battle_music.mp3) loops during Battle mode and Tutorial mode.
// Ball collision (ball_collision_sound.wav) uses setPan: screen-left = left ear (-1),
// screen-right = right ear (+1), mapped from the collision world X position.
// =============================================================================

SoundManager g_SoundManager;

SoundManager::SoundManager()
    : fmodSystem(nullptr), menuMusic(nullptr), battleMusic(nullptr), creditsMusic(nullptr),
      selectionSound(nullptr), ballCollisionSfx(nullptr), personaSummonSfx(nullptr),
      musicChannel(nullptr), currentTrack(nullptr),
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
        MAINMENU_MUSIC_FILE,
        FMOD_LOOP_NORMAL | FMOD_DEFAULT,
        nullptr,
        &menuTrack);
    if (result != FMOD_OK) {
        menuTrack = nullptr;
    }

    FMOD::Sound* battleTrack = nullptr;
    result = system->createSound(
        BATTLE_MUSIC_FILE,
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

    FMOD::Sound* ballCollision = nullptr;
    result = system->createSound(
        BALL_COLLISION_SOUND,
        FMOD_DEFAULT,
        nullptr,
        &ballCollision);
    if (result != FMOD_OK) {
        ballCollision = nullptr;
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
        if (ballCollision) {
            ballCollision->release();
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
    ballCollisionSfx = ballCollision;
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

// Map screen X to FMOD stereo pan: 0 = left edge (-1), SCREEN_WIDTH = right edge (+1).
static float StereoPanFromWorldX(float worldX) {
    float t = worldX / (float)SCREEN_WIDTH;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return t * 2.0f - 1.0f;
}

void SoundManager::PlayBallCollisionSfx(float worldX) {
    if (!isInitialised || ballCollisionSfx == nullptr) {
        return;
    }

    FMOD::System* system = static_cast<FMOD::System*>(fmodSystem);
    FMOD::Sound* sound = static_cast<FMOD::Sound*>(ballCollisionSfx);
    FMOD::Channel* channel = nullptr;

    if (system->playSound(sound, nullptr, false, &channel) == FMOD_OK && channel != nullptr) {
        channel->setPan(StereoPanFromWorldX(worldX));
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

    if (ballCollisionSfx != nullptr) {
        static_cast<FMOD::Sound*>(ballCollisionSfx)->release();
        ballCollisionSfx = nullptr;
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
