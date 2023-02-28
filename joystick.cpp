// Module that contains joystick functions

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <pthread.h>
#include <iostream>
#include <chrono>

#include "joystick.h"
#include "utils.h"

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
static bool cleanupFlag = false;
static ShutdownManager* pShutdownManager = nullptr;

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
    sleepForMs(100);
	while (true) {
        if (cleanupFlag) {
            break;
        }
        enum direction currentDirection = Joystick_checkWhichDirectionIsPressed();
        // if (100.0 < (double(clock() - upDirectionTimer) / CLOCKS_PER_SEC * 1000)) {
        //     printf("%f\n", (double(clock() - upDirectionTimer) / CLOCKS_PER_SEC * 1000));
        // }
        
        if (currentDirection == up && 100.0 < (double(clock() - upDirectionTimer) / CLOCKS_PER_SEC * 1000)) {
            printf("up command here");
            printf("%f", (double(clock() - upDirectionTimer) / CLOCKS_PER_SEC * 1000));
            upDirectionTimer = clock();
        } else if (currentDirection == down) {
            printf("down command here");
        } else if (currentDirection == left) {
            printf("left command here");
        } else if (currentDirection == right) {
            printf("right command here");
        } else if (currentDirection == pushed) {
            printf("pushed command here");
        }
    }
    return 0;
}

void Joystick_initializeJoystick(ShutdownManager* pShutdownManagerArg)
{
    if (pShutdownManagerArg == nullptr) {
        return;
    }
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