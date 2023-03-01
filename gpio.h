// This modules provides functions to interface with GPIO (General Purpose Input and Output).
// This includes functions to prepare the GPIO pins for proper use, as well as reading and writing from/to them.
#ifndef GPIO_H_
#define GPIO_H_

#include <string>

#include <stddef.h>

#include "int_typedefs.h"

#define GPIO_MAX_MODE_LEN 64

#define GPIO_READ_ERR -1

#define GPIO_EXPORT_PATH "/sys/class/gpio/export"
#define GPIO_PIN_PATH_PREFIX "/sys/class/gpio/gpio"
#define GPIO_CONFIG_PIN_PATH "/usr/bin/config-pin"

typedef struct {
    const char* header;
    const char* pin;
} GpioInfo;

typedef struct {
    GpioInfo gpioInfo;
    const char* linuxPin;
} GpioLinuxInfo;

void Gpio_exportPin(const char* linuxPin);
int Gpio_precheckSetPinMode(const char* header, const char* pin, const char* mode, size_t maxModeLen);
int Gpio_setPinMode(const char* header, const char* pin, const char* mode);
int Gpio_queryPinMode(const char* header, const char* pin, char* outMode, size_t maxModeLen);
void Gpio_configIo(const char* linuxPin, bool isInput);
int Gpio_read(const char* linuxPin);
void Gpio_write(const char* linuxPin, uint8 value);

#endif