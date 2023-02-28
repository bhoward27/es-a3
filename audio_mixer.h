// CITATION: This file is based off Dr. Brian Fraser's provided code.
// TODO: Add comment explaining the file.
#ifndef AUDIO_MIXER_H_
#define AUDIO_MIXER_H_

#include <alsa/asoundlib.h>
#include <array>
#include <thread>
#include <mutex>
#include <vector>

#include "shutdown_manager.h"

typedef struct {
	// A pointer to a previously allocated sound bite/clip (wavedata_t struct).
	// Note that many different sound-bite slots could share the same pointer
	// (overlapping cymbal crashes, for example)
	std::vector<short>* pSound = nullptr;

	// The offset into the pData of pSound. Indicates how much of the
	// sound has already been played (and hence where to start playing next).
	std::vector<short>::size_type playbackPos = 0;
} playbackSound_t;

struct SoundCollection {
    std::vector<short> bassDrum;
    std::vector<short> hiHat;
    std::vector<short> snare;
    std::vector<short> cymbal;
    std::vector<short> clave;
    std::vector<short> tomDrum;
};

class AudioMixer {
    public:
        static const int defaultVolume = 80;
        static const int maxVolume = 100;
        static const int sampleRateHz = 44100;
        static const int numChannels = 1;
        static const size_t sampleSize = sizeof(short); // We assume frame size is 1 since numChannels = 1.

        // Currently active (waiting to be played) sound bites/clips
        static const int maxAudioClips = 30;

        // Please don't modify outside of this class.
        // TODO: Could use enums instead to choose sound if really want to prevent modification of this variable.
        SoundCollection sound;

        AudioMixer(ShutdownManager* pShutdownManager);
        ~AudioMixer();
        void queueSound(std::vector<short>* pNewClip);
        int getVolume();
        void setVolume(int newVolume);

    private:
        snd_pcm_t* handle = nullptr;
        unsigned long playbackBufferSize = 0;
        std::vector<short> playbackBuffer;
        std::array<playbackSound_t, maxAudioClips> audioClips;
        std::thread playbackThread;
        std::mutex lock;
        int volume = 0;
        ShutdownManager* pShutdownManager = nullptr;

        void readWav(std::string fileName, std::vector<short>& sound);
        void fillPlaybackBuffer();
        void run();
};

#endif