#include <iostream>

#include "audio_mixer.h"
#include "shutdown_manager.h"
#include "beat_player.h"
#include "utils.h"

int main() {
    std::cout << "Hello BeagleBone!\n";

    ShutdownManager shutdownManager;
    AudioMixer mixer(&shutdownManager);
    BeatPlayer beatPlayer(&shutdownManager, &mixer);

    beatPlayer.setBpm(300);
    beatPlayer.play(Beat::alternate);
    sleepForMs(1000);
    beatPlayer.stop();
    beatPlayer.setBpm(40);
    beatPlayer.play(Beat::alternate);
    sleepForMs(1000);
    beatPlayer.play(Beat::standard);
    sleepForMs(1000);
    beatPlayer.stop();
    shutdownManager.requestShutdown();
    mixer.waitForShutdown();

    return 0;
}