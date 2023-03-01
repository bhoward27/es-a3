// Some code provided by Dr. Fraser

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "terminalOutput.h"
#include "utils.h"
#include "periodTimer.h"

#define TIME_TO_SLEEP_BETWEEN_OUTPUTS 1000

static pthread_t samplerId;
static int cleanupFlag = 0;
static BeatPlayer* pBeatPlayer = nullptr;
static AudioMixer* pAudioMixer = nullptr;
Period_statistics_t *pAudioStats = nullptr;
// Period_statistics_t *pAccelStats = nullptr;

static void *terminalOutputThread(void *args)
{
	while(1) {
        sleepForMs(TIME_TO_SLEEP_BETWEEN_OUTPUTS);
		if (cleanupFlag){
			break;
		}

        Period_getStatisticsAndClear(PERIOD_EVENT_FILL_BUFFER, pAudioStats);
        // Period_getStatisticsAndClear(PERIOD_EVENT_FILL_BUFFER, pAccelStats);
		printf("M%d %dbpm vol:%d  Audio[%f, %f] avg %f/%d\n"
        , (int)pBeatPlayer->getBeat()
        , pBeatPlayer->getBpm()
        , pAudioMixer->getVolume()
        , pAudioStats->minPeriodInMs, pAudioStats->maxPeriodInMs
        , pAudioStats->avgPeriodInMs, pAudioStats->numSamples
        // , pAccelStats->minPeriodInMs, pAccelStats->maxPeriodInMs
        // , pAccelStats->avgPeriodInMs, pAccelStats->numSamples
        );
    }
	return NULL;
}

void TerminalOutput_initialize(AudioMixer* pAudioMixerArg, BeatPlayer* pBeatPlayerArg)
{
    if (pAudioMixerArg == nullptr || pBeatPlayerArg == nullptr) {
        return;
    }

    pAudioMixer = pAudioMixerArg;
    pBeatPlayer = pBeatPlayerArg;

    pthread_create(&samplerId, NULL, &terminalOutputThread, NULL);
    // Learned how to allocate memory for a typedef struct from this link: https://stackoverflow.com/questions/4252180/using-malloc-in-c-to-allocate-space-for-a-typedefd-type
    pAudioStats = new(Period_statistics_t);
    // pAccelStats = new(Period_statistics_t);
}

void TerminalOutput_cleanup(void)
{
    cleanupFlag = 1;
    pthread_join(samplerId, NULL);
    free(pAudioStats);
    // free(pAccelStats);
}