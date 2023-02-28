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
    Joystick_initializeJoystick(&shutdownManager);
    AudioMixer mixer(&shutdownManager);
    BeatPlayer beatPlayer(&shutdownManager, &mixer);
    UdpServer_initialize(&shutdownManager, &mixer, &beatPlayer);

    UdpServer_cleanup();
    Joystick_cleanupJoystick();
    printf("Done!\n");

    return 0;
}