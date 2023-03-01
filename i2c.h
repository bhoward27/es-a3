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

// const GpioInfo I2c_bus1GpioPinInfo[I2C_BUS_NUM_PINS] = {
//     {I2C_BUS_1_GPIO_HEADER, I2C_BUS_1_GPIO_DATA_PIN},
//     {I2C_BUS_1_GPIO_HEADER, I2C_BUS_1_GPIO_CLOCK_PIN}
// };

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

        I2c(uint8 busNumber, uint8 gpioExtenderAddress)
        {
            // We assume just assume that we're using bus 1. Didn't have time to do it properly.

            int numSuccessful = 0;
            for (int i = 0; i < numPins; i++) {
                int res = Gpio_precheckSetPinMode(bus1GpioPinInfo[i].header, bus1GpioPinInfo[i].pin, "i2c", GPIO_MAX_MODE_LEN);
                if (res == COMMAND_SUCCESS) numSuccessful++;
            }
            // if (numSuccessful != I2C_BUS_NUM_PINS) {
            //     LOG(LOG_LEVEL_WARN, "%s(%p, %u, %u) FAILED.\n", __func__, busGpioInfo, busNumber, gpioExtenderAddress);
            // }
            if (numSuccessful != numPins) {
                throw std::runtime_error("Failed to set pin mode.");
            }

            fd = openBus(busNumber, gpioExtenderAddress);

            // Enable output on all pins for GPIO extender.
            numSuccessful = 0;
            for (uint8 i = 0; i < 2; i++) {
                // Write zeroes to register address 0x00 and 0x01. (NOTE: Zed Cape Red would need different addresses).
                int res = write(i, 0x00);
                if (res == OK) numSuccessful++;
            }
            if (numSuccessful != numPins) {
                throw std::runtime_error("Failed to write to I2C.");
            }
            // if (numSuccessful == numPins) {
            //     LOG(LOG_LEVEL_DEBUG, "%s(%p, %u, %u) SUCCEEDED.\n", __func__, busGpioInfo, busNumber, gpioExtenderAddress);
            // }
            // else {
            //     LOG(LOG_LEVEL_WARN, "%s(%p, %u, %u) FAILED.\n", __func__, busGpioInfo, busNumber, gpioExtenderAddress);
            //     return -1;
            // }
        }

        ~I2c()
        {
            close(fd);
            // if (res == -1) {
            //     SYS_WARN("Failed to close I2C bus with file descriptor = %d.\n", fd);
            //     return ERR_CLOSE;
            // }
            // return OK;
        }

        int write(uint8 registerAddress, uint8 value)
        {
            uint8 buffer[2];
            buffer[0] = registerAddress;
            buffer[1] = value;
            // ::write means use the write I'm including in from unistd.h, as opposed to I2c::write.
            int res = ::write(fd, buffer, sizeof(buffer));
            if (res != sizeof(buffer)) {
                // SYS_WARN("%s(%d, %u, %u) failed.\n", __func__, fd, registerAddress, value);
                return -1; // Error
            }
            return OK; // OK
        }

        void read(uint8 registerAddress, uint8 outBuffer[], int numBytes)
        {
            // char* buffer;
            // size_t count;
            // int res = ::read(fd, buffer, count);

            // To read a register, must first write the address
            int res = ::write(fd, &registerAddress, sizeof(registerAddress));
            if (res != sizeof(registerAddress)) {
                perror("I2C: Unable to write to i2c register.");
                exit(1);
            }
            // Now read the value and return it
            res = ::read(fd, outBuffer, numBytes);
            if (res != numBytes) {
                perror("I2C: Unable to read from i2c register");
                exit(1);
            }
        }

    private:
        int openBus(uint8 busNumber, uint8 deviceAddress)
        {
            char busFilePath[MEDIUM_STRING_LEN];
            snprintf(busFilePath, MEDIUM_STRING_LEN, "%s%u", devFilePathPrefix, busNumber);
            int i2cFileDesc = open(busFilePath, O_RDWR);
            if (i2cFileDesc == -1) {
                throw std::runtime_error("Failed to open '" + std::string(busFilePath) + "'.");
            }

            int result = ioctl(i2cFileDesc, I2C_SLAVE, deviceAddress);
            if (result < 0) {
                throw std::runtime_error(
                    "Failed to set I2C device (base 10) " + std::to_string(deviceAddress) + " to slave address."
                );
            }

            return i2cFileDesc;
        }
};

#endif