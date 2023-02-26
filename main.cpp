#include <iostream>

#include "audio_mixer.h"
#include "shutdown_manager.h"
#include "utils.h"

int main() {
    std::cout << "Hello BeagleBone!\n";

    ShutdownManager shutdownManager;
    AudioMixer mixer(&shutdownManager);

    // sleepForMs(1000);
    // shutdownManager.requestShutdown();
    while (true) {
        for (int i = 0; i < 5; i++) {
            mixer.queueSound(&mixer.sound.bassDrum);
            sleepForMs(100);
            mixer.queueSound(&mixer.sound.hiHat);
            sleepForMs(100);
            mixer.queueSound(&mixer.sound.snare);
        }

        sleepForMs(1000);
    }

    mixer.waitForShutdown();

    return 0;
}