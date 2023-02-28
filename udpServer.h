// Module allows you to initialize the udpserver in a thread which can reply to commands
// The server will provide access to the following commands
// count, length, history, get #, dips, and stop
// the stop command is how the whole application will stop and clean up other modules

#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <pthread.h>

#include "shutdown_manager.h"
#include "beat_player.h"
#include "audio_mixer.h"

// Initializes the udpServer and allows it to take commands
void UdpServer_initialize(ShutdownManager* pShutdownManagerArg, AudioMixer* pAudioMixerArg, BeatPlayer* pBeatPlayerArg);

// Cleans up and closes the udpServer
void UdpServer_cleanup(void);

#endif