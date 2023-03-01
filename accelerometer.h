// Provides the Accelerometer class, which reads from accelerometer once every 50 ms.
#ifndef ACCELEROMETER_H_
#define ACCELEROMETER_H_

#include "i2c.h"
#include "shutdown_manager.h"

#include <iostream>
#include <thread>
#include <atomic>

struct Acceleration {
    int16 x, y, z;
};

struct BooleanAcceleration {
    bool x, y, z;
};

struct AtomicBooleanAcceleration {
    std::atomic<bool> x, y, z;
};

enum class Axis {x, y, z};

class Accelerometer {
    public:
        static const uint8 busNumber = 1;
        static const uint8 deviceAddress = 0x1C;
        static const uint8 ctrlReg1Address = 0x2A;
        static const uint8 active = 1;

        static const uint8 statusRegisterAddress = 0x00;
        static const uint8 outXMsbAddress = 0x01;
        static const uint8 outXLsbAddress = 0x02;
        static const uint8 outYMsbAddress = 0x03;
        static const uint8 outYLsbAddress = 0x04;
        static const uint8 outZMsbAddress = 0x05;
        static const uint8 outZLsbAddress = 0x06;

        static const int16 restingForce = 200;
        static const int16 restingForceZ = 17000;
        static const int16 minDelta = 7000;
        static const int16 minPositiveForce = restingForce + minDelta;
        static const int16 minNegativeForce = -minPositiveForce;
        static const int16 minPositiveForceZ = restingForceZ + minDelta;
        static const int16 minNegativeForceZ = -minPositiveForceZ;

        Accelerometer(ShutdownManager* pShutdownManager);
        ~Accelerometer();
        Acceleration readRaw();
        BooleanAcceleration getBoolAccel();

    private:
        I2c i2c;
        AtomicBooleanAcceleration boolAccel;
        ShutdownManager* pShutdownManager = nullptr;
        std::thread thread;

        void run();
};

#endif