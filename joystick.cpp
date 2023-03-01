// Module that contains joystick functions

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <pthread.h>
#include <iostream>
#include <chrono>

#include "joystick.h"
#include "utils.h"
#include "joystick.h"

#define JOYSTICK_UP_DIRECTION_FILE "/sys/class/gpio/gpio26/direction"
#define JOYSTICK_DOWN_DIRECTION_FILE "/sys/class/gpio/gpio46/direction"
#define JOYSTICK_LEFT_DIRECTION_FILE "/sys/class/gpio/gpio65/direction"
#define JOYSTICK_RIGHT_DIRECTION_FILE "/sys/class/gpio/gpio47/direction"
#define JOYSTICK_PUSHED_DIRECTION_FILE "/sys/class/gpio/gpio27/direction"

#define JOYSTICK_UP_VALUE_FILE "/sys/class/gpio/gpio26/value"
#define JOYSTICK_DOWN_VALUE_FILE "/sys/class/gpio/gpio46/value"
#define JOYSTICK_LEFT_VALUE_FILE "/sys/class/gpio/gpio65/value"
#define JOYSTICK_RIGHT_VALUE_FILE "/sys/class/gpio/gpio47/value"
#define JOYSTICK_PUSHED_VALUE_FILE "/sys/class/gpio/gpio27/value"

static pthread_t samplerId;
// Learned how to make ms timer from this link: https://www.reddit.com/r/learnprogramming/comments/1dlxqv/comment/c9rksma/
static clock_t upDirectionTimer = clock();
static clock_t downDirectionTimer = clock();
static clock_t leftDirectionTimer = clock();
static clock_t rightDirectionTimer = clock();
static clock_t pushedDirectionTimer = clock();

static bool cleanupFlag = false;
static ShutdownManager* pShutdownManager = nullptr;
static BeatPlayer* pBeatPlayer = nullptr;
static AudioMixer* pAudioMixer = nullptr;

// The function below was provided by Dr. Brian Fraser
static void Joystick_runCommand(std::string command)
{
    // Execute the shell command (output into pipe)
    // Learned how to convert string to c string using this link: https://www.geeksforgeeks.org/convert-string-char-array-cpp/
    FILE *pipe = popen(command.c_str(), "r");
    // Ignore output of the command; but consume it
    // so we don't get an error when closing the pipe.
    char buffer[1024];
    while (!feof(pipe) && !ferror(pipe)) {
        if (fgets(buffer, sizeof(buffer), pipe) == NULL)
            break;
        // printf("--> %s", buffer);  // Uncomment for debugging
    }
    // Get the exit code from the pipe; non-zero is an error:
    int exitCode = WEXITSTATUS(pclose(pipe));
    if (exitCode != 0) {
        perror("Unable to execute command:");
        printf("  command:   %s\n", command.c_str());
        printf("  exit code: %d\n", exitCode);
    }
}

static void Joystick_setGPIODirectionToIn(std::string directionFile)
{
    FILE *dFile = fopen(directionFile.c_str(), "w");
    if (dFile == NULL) {
        printf("ERROR: Unable to open direction file.\n");
        exit(1);
    }
    fprintf(dFile, "%s", "in");
    fclose(dFile);
}

static int Joystick_isDirectionPressed(std::string valueFile)
{
    FILE *vFile = fopen(valueFile.c_str(), "r");
    if (vFile == NULL) {
        printf("ERROR: Unable to open value file.\n");
        return 0;
    }

    const int MAX_LENGTH = 1024;
    char buff[MAX_LENGTH];
    fgets(buff, MAX_LENGTH, vFile);
    fclose(vFile);
    if (*buff == '0'){
        return 1;
    }

    return 0;
}

enum direction Joystick_checkWhichDirectionIsPressed(void)
{
    if (Joystick_isDirectionPressed(JOYSTICK_UP_VALUE_FILE)) {
        enum direction returnValue = up;
        return returnValue;
    } else if (Joystick_isDirectionPressed(JOYSTICK_DOWN_VALUE_FILE)) {
        enum direction returnValue = down;
        return returnValue;
    } else if (Joystick_isDirectionPressed(JOYSTICK_LEFT_VALUE_FILE)) {
        enum direction returnValue = left;
        return returnValue;
    } else if (Joystick_isDirectionPressed(JOYSTICK_RIGHT_VALUE_FILE)) {
        enum direction returnValue = right;
        return returnValue;
    } else if (Joystick_isDirectionPressed(JOYSTICK_PUSHED_VALUE_FILE)) {
        enum direction returnValue = pushed;
        return returnValue;
    } else {
        enum direction returnValue = none;
        return returnValue;
    }
}

static void *joystickThread(void *args)
{
	while (true) {
        sleepForMs(10);
        if (cleanupFlag) {
            break;
        }

        enum direction currentDirection = Joystick_checkWhichDirectionIsPressed();
        if (currentDirection == up && 90.0 < (double(clock() - upDirectionTimer) / CLOCKS_PER_SEC * 1000)) {
            pAudioMixer->increaseVolume();
            upDirectionTimer = clock();
        } else if (currentDirection == down && 90.0 < (double(clock() - downDirectionTimer) / CLOCKS_PER_SEC * 1000)) {
            pAudioMixer->decreaseVolume();
            downDirectionTimer = clock();
        } else if (currentDirection == left && 90.0 < (double(clock() - leftDirectionTimer) / CLOCKS_PER_SEC * 1000)) {
            pBeatPlayer->decreaseTempo();
            leftDirectionTimer = clock();
        } else if (currentDirection == right && 90.0 < (double(clock() - rightDirectionTimer) / CLOCKS_PER_SEC * 1000)) {
            pBeatPlayer->increaseTempo();
            rightDirectionTimer = clock();
        } else if (currentDirection == pushed && 150.0 < (double(clock() - pushedDirectionTimer) / CLOCKS_PER_SEC * 1000)) {
            Beat currentBeat = pBeatPlayer->getBeat();
            // Learned how to convert int to enum in c++ from this link: https://stackoverflow.com/questions/11452920/how-to-cast-int-to-enum-in-c
            pBeatPlayer->play(static_cast<Beat>(((int)currentBeat+1)%3));
            pushedDirectionTimer = clock();
        }
    }
    return 0;
}

void Joystick_initializeJoystick(ShutdownManager* pShutdownManagerArg, AudioMixer* pAudioMixerArg, BeatPlayer* pBeatPlayerArg)
{
    if (pShutdownManagerArg == nullptr || pAudioMixerArg == nullptr || pBeatPlayerArg == nullptr) {
        return;
    }

    pAudioMixer = pAudioMixerArg;
    pBeatPlayer = pBeatPlayerArg;
    pShutdownManager = pShutdownManagerArg;

    Joystick_runCommand("config-pin p8.14 gpio");
    Joystick_runCommand("config-pin p8.15 gpio");
    Joystick_runCommand("config-pin p8.16 gpio");
    Joystick_runCommand("config-pin p8.18 gpio");
    Joystick_runCommand("config-pin p8.17 gpio");

    Joystick_setGPIODirectionToIn(JOYSTICK_UP_DIRECTION_FILE);
    Joystick_setGPIODirectionToIn(JOYSTICK_RIGHT_DIRECTION_FILE);
    Joystick_setGPIODirectionToIn(JOYSTICK_DOWN_DIRECTION_FILE);
    Joystick_setGPIODirectionToIn(JOYSTICK_LEFT_DIRECTION_FILE);
    Joystick_setGPIODirectionToIn(JOYSTICK_PUSHED_DIRECTION_FILE);

    pthread_create(&samplerId, NULL, &joystickThread, NULL);
}

void Joystick_cleanupJoystick(void)
{
    cleanupFlag = true;
    pthread_join(samplerId, NULL);
}