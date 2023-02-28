#include <iostream>

#include "audio_mixer.h"
#include "shutdown_manager.h"
#include "beat_player.h"
#include "utils.h"
#include "udpServer.h"
#include "joystick.h"

int main() {
    std::cout << "Hello BeagleBone!\n";

    ShutdownManager shutdownManager;
    UdpServer_initialize(&shutdownManager);
    Joystick_initializeJoystick(&shutdownManager);
    AudioMixer mixer(&shutdownManager);
    BeatPlayer beatPlayer(&shutdownManager, &mixer);

    beatPlayer.play(Beat::standard);

    // int volume = mixer.getVolume();
    // std::cout << "Volume = " << volume << std::endl;
    // sleepForMs(500);
    // while (true) {
    //     volume = mixer.increaseVolume();
    //     std::cout << "Volume = " << volume << std::endl;
    //     sleepForMs(500);
    // }
    int bpm = beatPlayer.getBpm();
    std::cout << "bpm = " << bpm << std::endl;
    sleepForMs(500);
    while (true) {
        bpm = beatPlayer.decreaseTempo();
        std::cout << "bpm = " << bpm << std::endl;
        sleepForMs(500);
    }

    beatPlayer.stop();

    UdpServer_cleanup();
    Joystick_cleanupJoystick();
    printf("Done!\n");

    return 0;
}