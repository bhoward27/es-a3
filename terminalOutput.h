// Module allows the initialization of the terminal output thread
// The thread will output to the terminal every second
// It will provide information about the samples taken since last output, pot value, history size, avg reading, dips, and timing jitter information

#ifndef TERMINAL_OUTPUT_H
#define TERMINAL_OUTPUT_H

#include "beat_player.h"
#include "audio_mixer.h"

// Initialize terminal output variables and thread
void TerminalOutput_initialize(AudioMixer* pAudioMixerArg, BeatPlayer* pBeatPlayerArg);

// Cleanup terminal output variables and thread
void TerminalOutput_cleanup(void);

#endif

