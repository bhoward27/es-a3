// This file provides the AirDrummer class -- so you can throw your BBG out the window with a sick beat playing.
#ifndef AIR_DRUMMER_H_
#define AIR_DRUMMER_H_

#include <stdexcept>
#include <thread>

#include "audio_mixer.h"
#include "accelerometer.h"
#include "shutdown_manager.h"

class AirDrummer {
    public:
        static const int64 debounce = 150;

        ShutdownManager* pShutdownManager = nullptr;
        Accelerometer* pAccel = nullptr;
        AudioMixer* pMixer = nullptr;

        AirDrummer(ShutdownManager* pShutdownManager, AudioMixer* pMixer, Accelerometer* pAccel);
        ~AirDrummer();

    private:
        std::thread xThread;
        std::thread yThread;
        std::thread zThread;

        void run(Axis axis);
};

#endif