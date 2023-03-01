#include "accelerometer.h"

Accelerometer::Accelerometer(ShutdownManager* pShutdownManager) : i2c(busNumber, deviceAddress)
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

Accelerometer::~Accelerometer()
{
    thread.join();
}

Acceleration Accelerometer::readRaw()
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

BooleanAcceleration Accelerometer::getBoolAccel()
{
    BooleanAcceleration boolAccel;
    boolAccel.x = this->boolAccel.x;
    boolAccel.y = this->boolAccel.y;
    boolAccel.z = this->boolAccel.z;
    return boolAccel;
}

void Accelerometer::run()
{
    while (!pShutdownManager->isShutdownRequested()) {
        Acceleration accel = readRaw();

        boolAccel.x = (accel.x >= minPositiveForce || accel.x <= minNegativeForce);
        boolAccel.y = (accel.y >= minPositiveForce || accel.y <= minNegativeForce);
        boolAccel.z = (accel.z >= minPositiveForceZ || accel.z <= minNegativeForceZ);

        sleepForMs(50);
    }
}