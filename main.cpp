#include <iostream>

#include "audio_mixer.h"
#include "shutdown_manager.h"
#include "beat_player.h"
#include "utils.h"
#include "udpServer.h"
#include "joystick.h"
#include "accelerometer.h"

int main() {
    std::cout << "Hello BeagleBone!\n";

    ShutdownManager shutdownManager;
    Joystick_initializeJoystick(&shutdownManager);
    AudioMixer mixer(&shutdownManager);
    BeatPlayer beatPlayer(&shutdownManager, &mixer);
    UdpServer_initialize(&shutdownManager, &mixer, &beatPlayer);
    Accelerometer accel;
    while (!shutdownManager.isShutdownRequested()) {
        accel.read();
        sleepForMs(250);
    }

    UdpServer_cleanup();
    Joystick_cleanupJoystick();
    printf("Done!\n");

    return 0;
}