#ifndef ACCELEROMETER_H_
#define ACCELEROMETER_H_

#include "i2c.h"
#include "shutdown_manager.h"

#include <iostream>
#include <thread>

struct Acceleration {
    int16 x, y, z;
};

struct BooleanAcceleration {
    bool x, y, z;
};

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
        static const int16 minDelta = 1000;
        static const int16 minPositiveForce = restingForce + minDelta;
        static const int16 minNegativeForce = -minPositiveForce;
        static const int16 minPositiveForceZ = restingForceZ + minDelta;
        static const int16 minNegativeForceZ = -minPositiveForceZ;

        Accelerometer(ShutdownManager* pShutdownManager) : i2c(busNumber, deviceAddress)
        {
            if (pShutdownManager == nullptr) {
                throw std::invalid_argument("pShutdownManager == nullptr.");
            }
            this->pShutdownManager = pShutdownManager;

            // Set CTRL_REG_1 to active.
            i2c.write(ctrlReg1Address, active);

            // Start thread to read the accelerometer every 50 ms or so.
            thread = std::thread([this] {run();});
        }

        Acceleration readRaw()
        {
            Acceleration accel = {0};
            const int numBytes = 7;
            uint8 buffer[numBytes];
            i2c.read(statusRegisterAddress, buffer, numBytes);

            accel.x = (buffer[outXMsbAddress] << 8) | (buffer[outXLsbAddress]);
            accel.y = (buffer[outYMsbAddress] << 8) | (buffer[outYLsbAddress]);
            accel.z = (buffer[outZMsbAddress] << 8) | (buffer[outZLsbAddress]);

            return accel;
        }

        void waitForShutdown()
        {
            thread.join();
        }


    private:
        I2c i2c;
        BooleanAcceleration boolAccel = {false};
        ShutdownManager* pShutdownManager = nullptr;
        std::thread thread;


        BooleanAcceleration getBoolAccel()
        {
            return boolAccel;
        }

        void run()
        {
            while (!pShutdownManager->isShutdownRequested()) {
                Acceleration accel = readRaw();

                boolAccel.x = (accel.x >= minPositiveForce || accel.x <= minNegativeForce);
                boolAccel.y = (accel.y >= minPositiveForce || accel.y <= minNegativeForce);
                boolAccel.z = (accel.z >= minPositiveForceZ || accel.z <= minNegativeForceZ);

                std::cout << "x = " << boolAccel.x << std::endl
                          << "y = " << boolAccel.y << std::endl
                          << "z = " << boolAccel.z << std::endl;

                sleepForMs(50);
            }
        }

};

#endif