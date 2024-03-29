// Header file for the joystick module that exposes the direction enum and a couple of functions.

#ifndef JOYSTICK_H
#define JOYSTICK_H

#include "shutdown_manager.h"
#include "beat_player.h"
#include "audio_mixer.h"

enum direction{none, up, down, left, right, pushed};

void Joystick_initializeJoystick(ShutdownManager* pShutdownManagerArg, AudioMixer* pAudioMixerArg, BeatPlayer* pBeatPlayerArg);
enum direction Joystick_checkWhichDirectionIsPressed(void);
void Joystick_cleanupJoystick(void);

#endif