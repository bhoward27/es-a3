#ifndef ACCELEROMETER_H_
#define ACCELEROMETER_H_

#include "i2c.h"

#include <iostream>

struct Acceleration {
    int16 x, y, z;
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

        Accelerometer() : i2c(busNumber, deviceAddress)
        {
            // Set CTRL_REG_1 to active.
            i2c.write(ctrlReg1Address, active);
        }

        Acceleration read()
        {
            Acceleration accel = {0};
            const int numBytes = 7;
            uint8 buffer[numBytes];
            i2c.read(statusRegisterAddress, buffer, numBytes);

            accel.x = (buffer[outXMsbAddress] << 8) | (buffer[outXLsbAddress]);
            accel.y = (buffer[outYMsbAddress] << 8) | (buffer[outYLsbAddress]);
            accel.z = (buffer[outZMsbAddress] << 8) | (buffer[outZLsbAddress]);

            std::cout << "accel.x = " << accel.x << std::endl
                      << "accel.y = " << accel.y << std::endl
                      << "accel.z = " << accel.z << std::endl;

            return accel;
        }

    private:
        I2c i2c;

};

#endif