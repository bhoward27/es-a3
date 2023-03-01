// Program entry point.
#include <iostream>

#include "audio_mixer.h"
#include "shutdown_manager.h"
#include "beat_player.h"
#include "utils.h"
#include "udpServer.h"
#include "joystick.h"
#include "accelerometer.h"
#include "air_drummer.h"
#include "terminalOutput.h"
#include "periodTimer.h"

int main()
{
    Period_init();
    ShutdownManager shutdownManager;
    AudioMixer mixer(&shutdownManager);
    BeatPlayer beatPlayer(&shutdownManager, &mixer);
    Joystick_initializeJoystick(&shutdownManager, &mixer, &beatPlayer);
    TerminalOutput_initialize(&mixer, &beatPlayer);
    UdpServer_initialize(&shutdownManager, &mixer, &beatPlayer);
    Accelerometer accel(&shutdownManager);
    AirDrummer(&shutdownManager, &mixer, &accel);

    UdpServer_cleanup();
    TerminalOutput_cleanup();
    Joystick_cleanupJoystick();
    Period_cleanup();

    return 0;
}