#include <string>
#include <cstring>
#include <cstdlib>
#include <algorithm>

#include <assert.h>
#include <stdio.h>

#include "gpio.h"
#include "utils.h"
#include "return_val.h"


int Gpio_precheckSetPinMode(const char* header, const char* pin, const char* mode, size_t maxCharsToCompare)
{
    /**
     * This function sets the pin's mode to mode only if the pin is not already set to that mode.
     *
     * It's recommended that mode be an array of size GPIO_MAX_MODE_LEN for more reliable string comparison internal to
     * to this function.
     */

    assert(mode);

    char currentMode[GPIO_MAX_MODE_LEN];
    int queryRes = Gpio_queryPinMode(header, pin, currentMode, GPIO_MAX_MODE_LEN);
    if (queryRes != COMMAND_SUCCESS) {
        return queryRes;
    }

    // Unintended consequence of passing min(...) to strncmp() can be illustrated in an example:
    // Suppose currentMode = "abc" and mode = "abz", and min(...) = 2. Then strncmp will incorrectly tell us that
    // currentMode == mode.
    // However, this unintended consequence is better than getting a segmentation fault, which could happen if passed
    // a non-null-terminated string. This is why it's recommended that mode be of size
    // GPIO_MAX_MODE_LEN = maxCharsToCompare.
    int isDifferentMode = strncmp(mode, currentMode, std::min((size_t)GPIO_MAX_MODE_LEN, maxCharsToCompare));
    if (isDifferentMode) {
        int setRes = Gpio_setPinMode(header, pin, mode);
        // if (setRes == COMMAND_SUCCESS) {
        //     LOG(LOG_LEVEL_DEBUG,
        //         "Changed GPIO mode for %s.%s from '%s' to '%s'.\n", header, pin, currentMode, mode);
        // }
        return setRes;
    }
    // else {
    //     LOG(LOG_LEVEL_DEBUG, "GPIO mode for %s.%s is already '%s'.\n", header, pin, mode);
    // }

    return COMMAND_SUCCESS;
}

int Gpio_setPinMode(const char* header, const char* pin, const char* mode)
{
    assert(header);
    assert(mode);
    assert(pin);

    char command[MEDIUM_STRING_LEN];
    snprintf(command, MEDIUM_STRING_LEN, "%s %s.%s %s", GPIO_CONFIG_PIN_PATH, header, pin, mode);
    int res = runCommand(command);

    return res;
}

int Gpio_queryPinMode(const char* header, const char* pin, char* outMode, size_t maxModeLen)
{
    assert(header);
    assert(pin);
    assert(outMode);

    char command[MEDIUM_STRING_LEN];
    // Citation: How to suppress blank lines in awk taken from here:
    // https://stackoverflow.com/questions/10347653/awk-remove-blank-lines
    snprintf(command, MEDIUM_STRING_LEN, "%s -q %s.%s | awk '!/^$/ {print $6}'", GPIO_CONFIG_PIN_PATH, header, pin);

    char output[MEDIUM_STRING_LEN];
    FILE* pipe = popen(command, "r");
    while (!feof(pipe) && !ferror(pipe)) {
        if (!fgets(output, sizeof(output), pipe)) {
            break;
        }
    }

    int exitCode = WEXITSTATUS(pclose(pipe));
    if (exitCode != COMMAND_SUCCESS) {
        // SYS_WARN("Command '%s' failed with exit code '%d'.\n", command, exitCode);
    }
    else {
        size_t maxLen = std::min(maxModeLen, (size_t) MEDIUM_STRING_LEN);
        size_t len = strnlen(output, maxLen - 1);

        // Want everything in output except the \n, which will the be the last character.
        strncpy(outMode, output, len - 1);
        outMode[len - 1] = '\0';
    }

    return exitCode;
}

void Gpio_exportPin(const char* linuxPin)
{
    // return overwriteFile(GPIO_EXPORT_PATH, linuxPin, false);
    const int maxTries = 3;
    const int64 sleepMs = 35;
    int ret;
    for (int i = 0; i < maxTries && ((ret = overwriteFile(GPIO_EXPORT_PATH, linuxPin, false)) != OK); i++) {
        // LOG(LOG_LEVEL_DEBUG, "Trying again in %lld ms...\n", sleepMs);
        sleepForMs(sleepMs);
    }

    // if (ret == OK) {
    //     LOG(LOG_LEVEL_DEBUG, "%s(%s) SUCCEEDED.\n\n", __func__, linuxPin);
    // }
    // else {
    //     LOG(LOG_LEVEL_DEBUG, "%s(%s) FAILED.\n\n", __func__, linuxPin);
    // }
}

void Gpio_configIo(const char* linuxPin, bool isInput)
{
    assert(linuxPin);

    char filePath[MEDIUM_STRING_LEN];
    snprintf(filePath, MEDIUM_STRING_LEN, "%s%s/direction", GPIO_PIN_PATH_PREFIX, linuxPin);
    int64 sleepMs = 35;
    int maxTries = 10;
    int ret = !OK;
    for (int i = 0; i < maxTries && ((ret = overwriteFile(filePath, ((isInput) ? "in" : "out"), false)) != OK); i++) {
        // LOG(LOG_LEVEL_DEBUG, "Trying again in %lld ms...\n", sleepMs);
        sleepForMs(sleepMs);
    }
    // if (ret == OK) {
    //     LOG(LOG_LEVEL_DEBUG, "%s(%s, %u) SUCCEEDED.\n\n", __func__, linuxPin, isInput);
    // }
    // else {
    //     LOG(LOG_LEVEL_DEBUG, "%s(%s, %u) FAILED.\n\n", __func__, linuxPin, isInput);
    // }
}

int Gpio_read(const char* linuxPin)
{
    assert(linuxPin);

    char filePath[MEDIUM_STRING_LEN];
    snprintf(filePath, MEDIUM_STRING_LEN, "%s%s/value", GPIO_PIN_PATH_PREFIX, linuxPin);

    char valueString[2];
    int res = readFile(filePath, (void*) valueString, sizeof(char), 1, false);
    if (res == OK) {
        // TODO: atoi also returns zero if the conversion failed! In this case, ideally would use a different method.
        return atoi(valueString);
    }
    else {
        return GPIO_READ_ERR;
    }
}

void Gpio_write(const char* linuxPin, uint8 value)
{
    assert(value == 0 || value == 1);
    assert(linuxPin);

    char filePath[MEDIUM_STRING_LEN];
    snprintf(filePath, MEDIUM_STRING_LEN, "%s%s/value", GPIO_PIN_PATH_PREFIX, linuxPin);
    char val[2];
    val[0] = (value) ? '1' : '0';
    val[1] = '\0';
    overwriteFile(filePath, val, false);
    // if (res != OK) {
    //     LOG(LOG_LEVEL_WARN, "%s(%p, %u) FAILED.\n", __func__, linuxPin, value);
    // }
}