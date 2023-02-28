// TODO: Add comment.

#ifndef BEAT_PLAYER_H_
#define BEAT_PLAYER_H_

#include <functional>
#include <thread>
#include <mutex>

#include "audio_mixer.h"
#include "shutdown_manager.h"
#include "utils.h"

enum class Beat {
    none,
    standard,
    alternate
};

class BeatPlayer {
    public:
        static const int defaultBpm = 120;
        static const int minBpm = 40;
        static const int maxBpm = 300;
        static const int bpmDelta = 5;

        BeatPlayer(ShutdownManager* pShutdownManager, AudioMixer* pMixer);
        void play(Beat beat);

        // Blocking. Whichever thread is using the beat player (has called play()) should call stop() as part of its
        // shutdown
        // procedure.
        void stop();
        int setBpm(int bpm);
        int getBpm();
        int increaseTempo();
        int decreaseTempo();

    private:
        ShutdownManager* pShutdownManager = nullptr;
        AudioMixer* pMixer = nullptr;
        bool isPlaying = false;
        int bpm = defaultBpm;
        Beat beat = Beat::none;
        std::thread loopThread;
        std::mutex lock;

        void playLoop(std::function<void()> playBeat);
        void playStandardBeat();
        void playAlternateBeat();
        int64 getHalfBeatTimeMs();
        int clampBpm(int bpm);
};

#endif