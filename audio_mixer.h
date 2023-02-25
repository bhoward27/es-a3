// CITATION: This file is based off Dr. Brian Fraser's provided code.
#ifndef AUDIO_MIXER_H_
#define AUDIO_MIXER_H_

#include <alsa/asoundlib.h>
#include <array>
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <cmath>

#include "shutdown_manager.h"
#include "utils.h"

// typedef struct {
// 	int numSamples;
// 	short *pData;
// } wavedata_t;

typedef struct {
	// A pointer to a previously allocated sound bite/clip (wavedata_t struct).
	// Note that many different sound-bite slots could share the same pointer
	// (overlapping cymbal crashes, for example)
	std::vector<short>* pSound = nullptr;

	// The offset into the pData of pSound. Indicates how much of the
	// sound has already been played (and hence where to start playing next).
	int playbackPosition = 0;
} playbackSound_t;

struct SoundCollection {
    std::vector<short> bassDrum;
    std::vector<short> hiHat;
    std::vector<short> snare;
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

        AudioMixer(ShutdownManager* shutdownManager)
        {

            this->shutdownManager = shutdownManager;

            // Read the wavFiles into memory.
            readWavFileIntoMemory("wave-files/100051__menegass__gui-drum-bd-hard.wav", sound.bassDrum);
            readWavFileIntoMemory("wave-files/100053__menegass__gui-drum-cc.wav", sound.hiHat);
            readWavFileIntoMemory("wave-files/100059__menegass__gui-drum-snare-soft.wav", sound.snare);

            // TODO: Set to default volume instead.
            // setVolume(defaultVolume);
            setVolume(maxVolume);

            // Open the PCM output
            int err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
            if (err < 0) {
                printf("Playback open error: %s\n", snd_strerror(err));
                exit(EXIT_FAILURE);
            }

            // Configure parameters of PCM output
            err = snd_pcm_set_params(handle,
                    SND_PCM_FORMAT_S16_LE,
                    SND_PCM_ACCESS_RW_INTERLEAVED,
                    numChannels,
                    sampleRateHz,
                    1,			// Allow software resampling. In our case, it's getting downsampled from 44.1 kHz to about 8 kHz.
                   50000);		// 0.05 seconds per buffer (50,000 microseconds)
                    // 1000000); // 1 second buffer (1,000,000 microseconds).
            if (err < 0) {
                printf("Playback open error: %s\n", snd_strerror(err));
                exit(EXIT_FAILURE);
            }

            // Allocate this software's playback buffer to be the same size as the
            // the hardware's playback buffers for efficient data transfers.
            // ..get info on the hardware buffers:
            unsigned long unusedBufferSize = 0;
            snd_pcm_get_params(handle, &unusedBufferSize, &playbackBufferSize);
            // ..allocate playback buffer:
            // playbackBuffer = (short*) malloc(playbackBufferSize * sizeof(*playbackBuffer));
            // Initialize buffer with zeros.
            playbackBuffer = std::vector<short>(playbackBufferSize, 0);

            // Launch playback thread:
            playbackThread = std::thread([this] {run();});
        }

        void waitForShutdown()
        {
            playbackThread.join();
        }

        ~AudioMixer()
        {
            printf("Stopping audio...\n");

            // Stop the PCM generation thread

            // Shutdown the PCM output, allowing any pending sound to play out (drain)
            snd_pcm_drain(handle);
            snd_pcm_close(handle);

            // Free playback buffer
            // (note that any wave files read into wavedata_t records must be freed
            //  in addition to this by calling AudioMixer_freeWaveFileData() on that struct.)
            // free(playbackBuffer);
            // playbackBuffer = nullptr;

            printf("Done stopping audio...\n");
            fflush(stdout);
        }

        void readWavFileIntoMemory(std::string fileName, std::vector<short>& sound)
        {
            // The PCM data in a wave file starts after the header:
            const int PCM_DATA_OFFSET = 44;

            // Open the wave file
            FILE *file = fopen(fileName.c_str(), "r");
            if (file == NULL) {
                fprintf(stderr, "ERROR: Unable to open file %s.\n", fileName.c_str());
                exit(EXIT_FAILURE);
            }

            // Get file size
            fseek(file, 0, SEEK_END);
            int sizeInBytes = ftell(file) - PCM_DATA_OFFSET;
            long numSamples = sizeInBytes / sampleSize;

            // Search to the start of the data in the file
            fseek(file, PCM_DATA_OFFSET, SEEK_SET);

            // Allocate space to hold all PCM data
            sound = std::vector<short>(numSamples);
            short* pTemp = new short[numSamples];

            // Read PCM data from wave file into memory
            int samplesRead = fread(pTemp, sampleSize, numSamples, file);
            if (samplesRead != numSamples) {
                std::cerr << "ERROR: Unable to read " << numSamples << " from file " << fileName << " (read " << samplesRead << ").\n";
                exit(EXIT_FAILURE);
            }

            std::copy_n(pTemp, numSamples, sound.data());

            delete[] pTemp;
            fclose(file);
        }

        // void freeWaveFileData(wavedata_t* pSound)
        // {
        //     pSound->numSamples = 0;
        //     free(pSound->pData);
        //     pSound->pData = NULL;
        // }

        // void queueSound(wavedata_t* pSound)
        // {
        //     // Ensure we are only being asked to play "good" sounds:
        //     assert(pSound->numSamples > 0);
        //     assert(pSound->pData);

        //     // Insert the sound by searching for an empty sound bite spot
        //     /*
        //     * REVISIT: Implement this:
        //     * 1. Since this may be called by other threads, and there is a thread
        //     *    processing the soundBites[] array, we must ensure access is threadsafe.
        //     * 2. Search through the soundBites[] array looking for a free slot.
        //     * 3. If a free slot is found, place the new sound file into that slot.
        //     *    Note: You are only copying a pointer, not the entire data of the wave file!
        //     * 4. After searching through all slots, if no free slot is found then print
        //     *    an error message to the console (and likely just return vs asserting/exiting
        //     *    because the application most likely doesn't want to crash just for
        //     *    not being able to play another wave file.
        //     */
        // }

        int getVolume()
        {
            // Return the cached volume; good enough unless someone is changing
            // the volume through other means and the cached value is out of date.
            return volume;
        }

        // CITATION: Dr. Fraser copied function from:
        // http://stackoverflow.com/questions/6787318/set-alsa-master-volume-from-c-code
        // Written by user "trenki".
        void setVolume(int newVolume)
        {
            // Ensure volume is reasonable; If so, cache it for later getVolume() calls.
            if (newVolume < 0 || newVolume > maxVolume) {
                printf("ERROR: Volume must be between 0 and 100.\n");
                return;
            }
            volume = newVolume;

            long min, max;
            snd_mixer_t *volHandle;
            snd_mixer_selem_id_t *sid;
            const char *card = "default";
            const char *selem_name = "PCM";

            snd_mixer_open(&volHandle, 0);
            snd_mixer_attach(volHandle, card);
            snd_mixer_selem_register(volHandle, NULL, NULL);
            snd_mixer_load(volHandle);

            snd_mixer_selem_id_alloca(&sid);
            snd_mixer_selem_id_set_index(sid, 0);
            snd_mixer_selem_id_set_name(sid, selem_name);
            snd_mixer_elem_t* elem = snd_mixer_find_selem(volHandle, sid);

            snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
            snd_mixer_selem_set_playback_volume_all(elem, volume * max / 100);

            snd_mixer_close(volHandle);
        }

    private:
        snd_pcm_t* handle = nullptr;
        unsigned long playbackBufferSize = 0;
        // short* playbackBuffer = nullptr; // TODO: Probably convert to an STL collection, or smart pointer
        std::vector<short> playbackBuffer;
        std::array<playbackSound_t, maxAudioClips> audioClips;
        SoundCollection sound;
        bool stopping = false;
        std::thread playbackThread;
        std::mutex audioMutex;
        int volume = 0;
        ShutdownManager* shutdownManager = nullptr;

        // Fill the `buff` array with new PCM values to output.
        //    `buff`: buffer to fill with new PCM data from sound bites.
        //    `size`: the number of values to store into playbackBuffer
        bool fillPlaybackBuffer(std::vector<short>& sineWave)
        {
            /*
            * REVISIT: Implement this
            * 1. Wipe the playbackBuffer to all 0's to clear any previous PCM data.
            *    Hint: use memset()
            * 2. Since this is called from a background thread, and soundBites[] array
            *    may be used by any other thread, must synchronize this.
            * 3. Loop through each slot in soundBites[], which are sounds that are either
            *    waiting to be played, or partially already played:
            *    - If the sound bite slot is unused, do nothing for this slot.
            *    - Otherwise "add" this sound bite's data to the play-back buffer
            *      (other sound bites needing to be played back will also add to the same data).
            *      * Record that this portion of the sound bite has been played back by incrementing
            *        the location inside the data where play-back currently is.
            *      * If you have now played back the entire sample, free the slot in the
            *        soundBites[] array.
            *
            * Notes on "adding" PCM samples:
            * - PCM is stored as signed shorts (between SHRT_MIN and SHRT_MAX).
            * - When adding values, ensure there is not an overflow. Any values which would
            *   greater than SHRT_MAX should be clipped to SHRT_MAX; likewise for underflow.
            * - Don't overflow any arrays!
            * - Efficiency matters here! The compiler may do quite a bit for you, but it doesn't
            *   hurt to keep it in mind. Here are some tips for efficiency and readability:
            *   * If, for each pass of the loop which "adds" you need to change a value inside
            *     a struct inside an array, it may be faster to first load the value into a local
            *      variable, increment this variable as needed throughout the loop, and then write it
            *     back into the struct inside the array after. For example:
            *           int offset = myArray[someIdx].value;
            *           for (int i =...; i < ...; i++) {
            *               offset ++;
            *           }
            *           myArray[someIdx].value = offset;
            *   * If you need a value in a number of places, try loading it into a local variable
            *          int someNum = myArray[someIdx].value;
            *          if (someNum < X || someNum > Y || someNum != Z) {
            *              someNum = 42;
            *          }
            *          ... use someNum vs myArray[someIdx].value;
            *
            */

            // TODO: Change to real stuff later. For now, just play the bass drum sound.
            // First, zero out the buffer. // TODO: Just zero out remaining samples AFTER buffer filled -- save a bit on copying.
            std::fill(playbackBuffer.begin(), playbackBuffer.end(), 0);
            // static std::vector<short>::size_type soundPos = 0;
            // std::copy_n(sound.bassDrum.begin() + soundPos, playbackBuffer.size(), playbackBuffer.begin());
            // soundPos += playbackBuffer.size();
            // if (soundPos > sound.bassDrum.size()) soundPos = 0;

            static std::vector<short>::size_type soundPos = 0;

            // std::copy_n(sineWave.begin() + soundPos, playbackBuffer.size(), playbackBuffer.begin());
            // soundPos += playbackBuffer.size();
            // if (soundPos >= sineWave.size()) soundPos = 0;
            std::copy_n(sound.hiHat.begin() + soundPos, playbackBuffer.size(), playbackBuffer.begin());
            soundPos += playbackBuffer.size();
            if (soundPos >= sound.hiHat.size()) {
                soundPos = 0;
                return false;
            }
            return true;
        }

        void run()
        {
            // Generate a 980 Hz sine wave.
            std::vector<short> sineWave;
            const double pi = std::acos(-1);
            const double numSamplesPerPeriod = 45;
            for (double i = 0, x = 0; i < 44100; i++, x++) {
                if (x >= numSamplesPerPeriod) x = 0;
                sineWave.push_back(32767 * std::sin(((2 * pi) / (numSamplesPerPeriod + 1)) * x));
            }

            while (!shutdownManager->isShutdownRequested()) {
                // Generate next block of audio
                // bool res = fillPlaybackBuffer(sineWave);
                fillPlaybackBuffer(sineWave);

                // Output the audio
                // This call is blocking. Returns after buffer has been completely transferred to the sound system.
                snd_pcm_sframes_t frames = snd_pcm_writei(handle, playbackBuffer.data(), playbackBuffer.size());

                // Check for (and handle) possible error conditions on output
                if (frames < 0) {
                    fprintf(stderr, "AudioMixer: writei() returned %li\n", frames);
                    frames = snd_pcm_recover(handle, frames, 1);
                }
                if (frames < 0) {
                    fprintf(stderr, "ERROR: Failed writing audio with snd_pcm_writei(): %li\n",
                            frames);
                    exit(EXIT_FAILURE);
                }
                if (frames > 0 && frames < static_cast<long>(playbackBufferSize)) {
                    printf("Short write (expected %li, wrote %li)\n",
                            playbackBufferSize, frames);
                }

                // TODO: Remove.
                // if (res) sleepForMs(1000);
            }
        }
};

#endif