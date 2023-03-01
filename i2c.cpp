#include "i2c.h"

I2c::I2c(uint8 busNumber, uint8 gpioExtenderAddress)
{
    // We assume just assume that we're using bus 1. Didn't have time to do it properly.

    int numSuccessful = 0;
    for (int i = 0; i < numPins; i++) {
        int res = Gpio_precheckSetPinMode(bus1GpioPinInfo[i].header, bus1GpioPinInfo[i].pin, "i2c", GPIO_MAX_MODE_LEN);
        if (res == COMMAND_SUCCESS) numSuccessful++;
    }
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
}

I2c::~I2c()
{
    close(fd);
}

int I2c::write(uint8 registerAddress, uint8 value)
{
    uint8 buffer[2];
    buffer[0] = registerAddress;
    buffer[1] = value;
    // ::write means use the write I'm including in from unistd.h, as opposed to I2c::write.
    int res = ::write(fd, buffer, sizeof(buffer));
    if (res != sizeof(buffer)) {
        return -1; // Error
    }
    return OK; // OK
}

void I2c::read(uint8 registerAddress, uint8 outBuffer[], int numBytes)
{
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

int I2c::openBus(uint8 busNumber, uint8 deviceAddress)
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