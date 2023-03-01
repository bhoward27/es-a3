#include <iostream>
#include <algorithm>
#include <exception>
#include <cmath>
#include <limits.h>

#include "audio_mixer.h"
#include "utils.h"
#include "periodTimer.h"

AudioMixer::AudioMixer(ShutdownManager* pShutdownManager)
{
    if (pShutdownManager == nullptr) {
        throw std::invalid_argument("pShutdownManager == nullptr.");
    }
    this->pShutdownManager = pShutdownManager;

    readWav("beatbox-wav-files/100051__menegass__gui-drum-bd-hard.wav", sound.bassDrum);
    readWav("beatbox-wav-files/100053__menegass__gui-drum-cc.wav", sound.hiHat);
    readWav("beatbox-wav-files/100059__menegass__gui-drum-snare-soft.wav", sound.snare);

    readWav("beatbox-wav-files/100055__menegass__gui-drum-co.wav", sound.cymbal);

    // This audio file was downloaded from https://freewavesamples.com/claves and converted to mono with
    // https://fconvert.com/audio/.
    readWav("beatbox-wav-files/claves-mono.wav", sound.clave);

    readWav("beatbox-wav-files/100063__menegass__gui-drum-tom-hi-soft.wav", sound.tomDrum);

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

AudioMixer::~AudioMixer()
{
    playbackThread.join();

    // Shutdown the PCM output, allowing any pending sound to play out (drain)
    snd_pcm_drain(handle);
    snd_pcm_close(handle);

    fflush(stdout);
}


void AudioMixer::queueSound(std::vector<short>* pNewClip)
{
    if (pNewClip == nullptr || pNewClip->size() == 0) {
        throw std::invalid_argument("Bad argument for pNewClip.");
    }
    bool foundFreeSlot = false;

    lock.lock();
    {
        for (auto& clip : audioClips) {
            if (clip.pSound == nullptr) {
                clip.pSound = pNewClip;
                foundFreeSlot = true;
                break;
            }
        }
    }
    lock.unlock();

    if (!foundFreeSlot) {
        std::cerr << "ERROR: All slots in audioClips are currently in use.\n";
    }
}

int AudioMixer::getVolume()
{
    // Return the cached volume; good enough unless someone is changing
    // the volume through other means and the cached value is out of date.
    int currentVolume;
    lock.lock();
    {
        currentVolume = volume;
    }
    lock.unlock();

    return currentVolume;
}

int AudioMixer::clampVolume(int volume)
{
    if (volume < minVolume) {
        return minVolume;
    }
    else if (volume > maxVolume) {
        return maxVolume;
    }
    else {
        return volume;
    }
}

// CITATION: Dr. Fraser copied function from:
// http://stackoverflow.com/questions/6787318/set-alsa-master-volume-from-c-code
// Written by user "trenki".
int AudioMixer::setVolume(int newVolume)
{
    newVolume = clampVolume(newVolume);

    lock.lock();
    {
        _setVolume(newVolume);
    }
    lock.unlock();

    return newVolume;
}

// Not thread-safe.
void AudioMixer::_setVolume(int newVolume)
{
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

int AudioMixer::increaseVolume()
{
    int newVolume;
    lock.lock();
    {
        newVolume = clampVolume(volume + volumeDelta);
        _setVolume(newVolume);
    }
    lock.unlock();
    return newVolume;
}

int AudioMixer::decreaseVolume()
{
    int newVolume;
    lock.lock();
    {
        newVolume = clampVolume(volume - volumeDelta);
        _setVolume(newVolume);
    }
    lock.unlock();
    return newVolume;
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

void AudioMixer::fillPlaybackBuffer()
{
    lock.lock();
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
        Period_markEvent(PERIOD_EVENT_FILL_BUFFER);
    }
    lock.unlock();
}

void AudioMixer::run()
{
    while (!pShutdownManager->isShutdownRequested()) {
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
