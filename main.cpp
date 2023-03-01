#include <iostream>

#include "audio_mixer.h"
#include "shutdown_manager.h"
#include "beat_player.h"
#include "utils.h"
#include "udpServer.h"
#include "joystick.h"
#include "terminalOutput.h"
#include "periodTimer.h"

int main() {
    std::cout << "Hello BeagleBone!\n";

    Period_init();
    ShutdownManager shutdownManager;
    AudioMixer mixer(&shutdownManager);
    BeatPlayer beatPlayer(&shutdownManager, &mixer);
    Joystick_initializeJoystick(&shutdownManager, &mixer, &beatPlayer);
    TerminalOutput_initialize(&mixer, &beatPlayer);
    UdpServer_initialize(&shutdownManager, &mixer, &beatPlayer);
        
    UdpServer_cleanup();
    TerminalOutput_cleanup();
    Joystick_cleanupJoystick();
    Period_cleanup();
    printf("Done!\n");

    return 0;
}