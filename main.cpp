#include <iostream>

#include "audio_mixer.h"
#include "shutdown_manager.h"
#include "beat_player.h"
#include "utils.h"
#include "udpServer.h"

int main() {
    std::cout << "Hello BeagleBone!\n";

    ShutdownManager shutdownManager;
    UdpServer_initialize(&shutdownManager);
    AudioMixer mixer(&shutdownManager);
    BeatPlayer beatPlayer(&shutdownManager, &mixer);

    // beatPlayer.setBpm(300);
    beatPlayer.play(Beat::alternate);
    sleepForMs(1000);
    beatPlayer.setBpm(40);
    sleepForMs(1000);
    // mixer.setVolume(20);
    beatPlayer.play(Beat::standard);
    sleepForMs(1000);

    beatPlayer.stop();

    printf("Cleaning everything up.\n");
    UdpServer_cleanup();
    printf("Done!\n");

    return 0;
}