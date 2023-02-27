#include <iostream>
#include <algorithm>
#include <cmath>
#include <limits.h>

#include "audio_mixer.h"
#include "utils.h"

AudioMixer::AudioMixer(ShutdownManager* shutdownManager)
{
    this->shutdownManager = shutdownManager;

    // Read the wavFiles into memory.
    readWav("wave-files/100051__menegass__gui-drum-bd-hard.wav", sound.bassDrum);
    readWav("wave-files/100053__menegass__gui-drum-cc.wav", sound.hiHat);
    readWav("wave-files/100059__menegass__gui-drum-snare-soft.wav", sound.snare);

    readWav("wave-files/100055__menegass__gui-drum-co.wav", sound.cymbal);
    readWav("wave-files/claves-mono.wav", sound.clave);
    readWav("wave-files/100063__menegass__gui-drum-tom-hi-soft.wav", sound.tomDrum);
    // 55
    // 63
    // 64
    // 65

    setVolume(defaultVolume);

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
            1,			// Allow software resampling.
            50000);		// 0.05 seconds per buffer (50,000 microseconds)
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
    // Initialize buffer with zeros.
    playbackBuffer = std::vector<short>(playbackBufferSize, 0);

    // Launch playback thread:
    playbackThread = std::thread([this] {run();});
}

void AudioMixer::waitForShutdown()
{
    playbackThread.join();
}

AudioMixer::~AudioMixer()
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


void AudioMixer::queueSound(std::vector<short>* pNewClip)
{
    // Insert the sound by searching for an empty sound bite spot
    /*
    * REVISIT: Implement this:
    * 1. Since this may be called by other threads, and there is a thread
    *    processing the soundBites[] array, we must ensure access is threadsafe.
    * 2. Search through the soundBites[] array looking for a free slot.
    * 3. If a free slot is found, place the new sound file into that slot.
    *    Note: You are only copying a pointer, not the entire data of the wave file!
    * 4. After searching through all slots, if no free slot is found then print
    *    an error message to the console (and likely just return vs asserting/exiting
    *    because the application most likely doesn't want to crash just for
    *    not being able to play another wave file.
    */
    if (pNewClip == nullptr || pNewClip->size() == 0) {
        throw std::invalid_argument("Bad argument for pNewClip.");
    }
    bool foundFreeSlot = false;

    audioMutex.lock();
    {
        for (auto& clip : audioClips) {
            if (clip.pSound == nullptr) {
                clip.pSound = pNewClip;
                foundFreeSlot = true;
                break;
            }
        }
    }
    audioMutex.unlock();

    if (!foundFreeSlot) {
        std::cerr << "ERROR: All slots in audioClips are currently in use.\n";
    }
}

int AudioMixer::getVolume()
{
    // Return the cached volume; good enough unless someone is changing
    // the volume through other means and the cached value is out of date.
    return volume;
}

// CITATION: Dr. Fraser copied function from:
// http://stackoverflow.com/questions/6787318/set-alsa-master-volume-from-c-code
// Written by user "trenki".
void AudioMixer::setVolume(int newVolume)
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

void AudioMixer::readWav(std::string fileName, std::vector<short>& sound)
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
    pTemp = nullptr;
    fclose(file);
}

 // Fill the `buff` array with new PCM values to output.
//    `buff`: buffer to fill with new PCM data from sound bites.
//    `size`: the number of values to store into playbackBuffer
void AudioMixer::fillPlaybackBuffer()
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
    // std::fill(playbackBuffer.begin(), playbackBuffer.end(), 0);
    // // static std::vector<short>::size_type soundPos = 0;
    // // std::copy_n(sound.bassDrum.begin() + soundPos, playbackBuffer.size(), playbackBuffer.begin());
    // // soundPos += playbackBuffer.size();
    // // if (soundPos > sound.bassDrum.size()) soundPos = 0;

    // static std::vector<short>::size_type soundPos = 0;

    // // std::copy_n(sineWave.begin() + soundPos, playbackBuffer.size(), playbackBuffer.begin());
    // // soundPos += playbackBuffer.size();
    // // if (soundPos >= sineWave.size()) soundPos = 0;
    // std::copy_n(sound.hiHat.begin() + soundPos, playbackBuffer.size(), playbackBuffer.begin());
    // soundPos += playbackBuffer.size();
    // if (soundPos >= sound.hiHat.size()) {
    //     soundPos = 0;
    // }

    audioMutex.lock();
    {
        for (auto& playbackSample : playbackBuffer) {
            int32 sum = 0;
            for (auto& clip : audioClips) {
                if (clip.pSound == nullptr) continue;

                if (clip.playbackPos >= clip.pSound->size()) {
                    clip.playbackPos = 0;
                    clip.pSound = nullptr;
                }
                else {
                    sum += (*clip.pSound)[clip.playbackPos++];
                }
            }
            if (sum > SHRT_MAX) {
                sum = SHRT_MAX;
            }
            else if (sum < SHRT_MIN) {
                sum = SHRT_MIN;
            }
            playbackSample = sum;
        }
    }
    audioMutex.unlock();
}

void AudioMixer::run()
{
    // Generate a 980 Hz sine wave.
    // std::vector<short> sineWave;
    // const double pi = std::acos(-1);
    // const double numSamplesPerPeriod = 45;
    // for (double i = 0, x = 0; i < 44100; i++, x++) {
    //     if (x >= numSamplesPerPeriod) x = 0;
    //     sineWave.push_back(32767 * std::sin(((2 * pi) / (numSamplesPerPeriod + 1)) * x));
    // }

    while (!shutdownManager->isShutdownRequested()) {
        // Generate next block of audio
        fillPlaybackBuffer();

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
    }
}
