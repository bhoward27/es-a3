#ifndef BEAT_PLAYER_H_
#define BEAT_PLAYER_H_

#include <exception>
#include <thread>
#include <functional>

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

        BeatPlayer(ShutdownManager* pShutdownManager, AudioMixer* pMixer)
        {
            if (pShutdownManager == nullptr) {
                throw std::invalid_argument("pShutdownManager == nullptr.");
            }
            if (pMixer == nullptr) {
                throw std::invalid_argument("pAudioMixer == nullptr.");
            }

            this->pShutdownManager = pShutdownManager;
            this->pMixer = pMixer;
        }

        // Blocking. Whichever thread is using the beat player should call this function as part of its shutdown
        // procedure.
        void stop()
        {
            if (isPlaying) {
                isPlaying = false;
                this->beat = Beat::none;
                loopThread.join();
            }
        }

        void play(Beat beat)
        {
            if (this->beat == beat) return;

            stop();
            this->beat = beat;

            if (beat != Beat::none) {
                isPlaying = true;
                switch (beat) {
                    case Beat::standard:
                        loopThread = std::thread([this] {
                                                            playLoop( [this] {
                                                                playStandardBeat();
                                                            });
                                                 });
                        break;
                    case Beat::alternate:
                        loopThread = std::thread([this] {
                                                            playLoop( [this] {
                                                                playAlternateBeat();
                                                            });
                                                 });
                        break;
                    default:
                        throw std::invalid_argument("Bad beat argument.");
                        break;
                }
            }
        }

        void setBpm(int bpm)
        {
            if (bpm < minBpm || bpm > maxBpm) return;

            this->bpm = bpm;
        }


    private:
        ShutdownManager* pShutdownManager = nullptr;
        AudioMixer* pMixer = nullptr;
        bool isPlaying = false;
        int bpm = defaultBpm;
        Beat beat = Beat::none;
        std::thread loopThread;

        void playLoop(const std::function<void()>& playBeat)
        {
            while (isPlaying && !pShutdownManager->isShutdownRequested()) {
                playBeat();
            }
        }

        void playStandardBeat()
        {
            int64 timeForHalfBeatMs = 30000 / bpm;

            pMixer->queueSound(&pMixer->sound.hiHat);
            pMixer->queueSound(&pMixer->sound.bassDrum);
            sleepForMs(timeForHalfBeatMs);

            pMixer->queueSound(&pMixer->sound.hiHat);
            sleepForMs(timeForHalfBeatMs);

            pMixer->queueSound(&pMixer->sound.hiHat);
            pMixer->queueSound(&pMixer->sound.snare);
            sleepForMs(timeForHalfBeatMs);

            pMixer->queueSound(&pMixer->sound.hiHat);
            sleepForMs(timeForHalfBeatMs);
        }

        void playAlternateBeat()
        {
            int64 halfBeat = 30000 / bpm;
            int64 fullBeat = 2 * halfBeat;

            pMixer->queueSound(&pMixer->sound.clave);
            pMixer->queueSound(&pMixer->sound.tomDrum);
            sleepForMs(halfBeat);

            pMixer->queueSound(&pMixer->sound.clave);
            sleepForMs(fullBeat);

            pMixer->queueSound(&pMixer->sound.clave);
            pMixer->queueSound(&pMixer->sound.cymbal);
            sleepForMs(halfBeat);

            pMixer->queueSound(&pMixer->sound.clave);
            sleepForMs(halfBeat);
        }

};

#endif