// The I2c class allows the user to create a connection to an I2C bus & device and read or write from it.
#ifndef I2C_H_
#define I2C_H_

#include <string>
#include <array>
#include <stdexcept>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdio.h>

#include "gpio.h"
#include "utils.h"
#include "return_val.h"

class I2c {
    public:
        static const int numPins = 2;
        static const uint8 bus1GpioExtenderAddress = 0x20;

        const char* bus1GpioHeader = "p9";
        const char* bus1GpioDataPin = "18";
        const char* bus1GpioClockPin = "17";
        const char* devFilePathPrefix = "/dev/i2c-";

        const GpioInfo bus1GpioPinInfo[numPins] {
            {bus1GpioHeader, bus1GpioDataPin},
            {bus1GpioHeader, bus1GpioClockPin}
        };

        int fd = -1;

        I2c(uint8 busNumber, uint8 gpioExtenderAddress);
        ~I2c();
        int write(uint8 registerAddress, uint8 value);
        void read(uint8 registerAddress, uint8 outBuffer[], int numBytes);

    private:
        int openBus(uint8 busNumber, uint8 deviceAddress);
};

#endif