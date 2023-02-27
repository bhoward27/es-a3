#include <exception>

#include "beat_player.h"

BeatPlayer::BeatPlayer(ShutdownManager* pShutdownManager, AudioMixer* pMixer)
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
void BeatPlayer::stop()
{
    lock.lock();
    if (isPlaying) {
        isPlaying = false;
        this->beat = Beat::none;
        lock.unlock();

        loopThread.join();
    }
    else {
        lock.unlock();
    }
}

void BeatPlayer::play(Beat beat)
{
    lock.lock();
    {
        bool isCurrentBeat = (this->beat == beat);
        if (isCurrentBeat) {
            lock.unlock();
            return;
        }

        if (isPlaying) {
            isPlaying = false;
            this->beat = Beat::none;
            lock.unlock();

            loopThread.join();

            lock.lock();
        }

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
                    lock.unlock();
                    throw std::invalid_argument("Bad beat argument.");
                    break;
            }
        }
    }
    lock.unlock();
}

void BeatPlayer::setBpm(int bpm)
{
    if (bpm < minBpm || bpm > maxBpm) return;

    lock.lock();
    {
        this->bpm = bpm;
    }
    lock.unlock();
}

void BeatPlayer::playLoop(std::function<void()> playBeat)
{
    bool isPlaying;
    lock.lock();
    {
        isPlaying = this->isPlaying;
    }
    lock.unlock();

    while (isPlaying && !pShutdownManager->isShutdownRequested()) {
        playBeat();

        lock.lock();
        {
            isPlaying = this->isPlaying;
        }
        lock.unlock();
    }
}

void BeatPlayer::playStandardBeat()
{
    int64 halfBeat = getHalfBeatTimeMs();
    pMixer->queueSound(&pMixer->sound.hiHat);
    pMixer->queueSound(&pMixer->sound.bassDrum);
    sleepForMs(halfBeat);

    pMixer->queueSound(&pMixer->sound.hiHat);
    sleepForMs(halfBeat);

    pMixer->queueSound(&pMixer->sound.hiHat);
    pMixer->queueSound(&pMixer->sound.snare);
    sleepForMs(halfBeat);

    pMixer->queueSound(&pMixer->sound.hiHat);
    sleepForMs(halfBeat);
}

void BeatPlayer::playAlternateBeat()
{
    int64 halfBeat = getHalfBeatTimeMs();
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

int64 BeatPlayer::getHalfBeatTimeMs()
{
    int64 halfBeat;
    lock.lock();
    {
        halfBeat = 30000 / bpm;
    }
    lock.unlock();
    return halfBeat;
}