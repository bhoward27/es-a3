#include "air_drummer.h"

AirDrummer::AirDrummer(ShutdownManager* pShutdownManager, AudioMixer* pMixer, Accelerometer* pAccel)
{
    if (pShutdownManager == nullptr) {
        throw std::invalid_argument("pShutdownManager == nullptr.");
    }
    this->pShutdownManager = pShutdownManager;
    if (pMixer == nullptr) {
        throw std::invalid_argument("pMixer == nullptr.");
    }
    this->pMixer = pMixer;
    if (pAccel == nullptr) {
        throw std::invalid_argument("pAccel == nullptr.");
    }
    this->pAccel = pAccel;

    // Spawn a thread for each axis.
    xThread = std::thread([this] {run(Axis::x);});
    yThread = std::thread([this] {run(Axis::y);});
    zThread = std::thread([this] {run(Axis::z);});
}

AirDrummer::~AirDrummer()
{
    zThread.join();
    yThread.join();
    xThread.join();
}

void AirDrummer::run(Axis axis)
{
    while (!pShutdownManager->isShutdownRequested()) {
        BooleanAcceleration accel = pAccel->getBoolAccel();
        switch (axis) {
            case Axis::x:
                if (accel.x) {
                    pMixer->queueSound(&pMixer->sound.hiHat);
                    sleepForMs(debounce);
                }
                break;
            case Axis::y:
                if (accel.y) {
                    pMixer->queueSound(&pMixer->sound.snare);
                    sleepForMs(debounce);
                }
                break;
            case Axis::z:
                if (accel.z) {
                    pMixer->queueSound(&pMixer->sound.bassDrum);
                    sleepForMs(debounce);
                }
                break;
            default:
                throw std::invalid_argument("Bad Axis argument.");
        }
    }
}