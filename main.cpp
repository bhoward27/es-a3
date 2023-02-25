#include <iostream>

#include "audio_mixer.h"
#include "shutdown_manager.h"

int main() {
    std::cout << "Hello BeagleBone!\n";

    ShutdownManager shutdownManager;
    AudioMixer mixer(&shutdownManager);

    mixer.waitForShutdown();

    return 0;
}