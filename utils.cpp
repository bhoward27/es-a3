#include <ctime>
#include <cassert>
#include <iostream>

#include "utils.h"

void sleepForMs(int64 delayInMs)
{
    const int64 NS_PER_MS = 1000 * 1000;
    const int64 NS_PER_SECOND = 1000000000;
    int64 delayNs = delayInMs * NS_PER_MS;
    int seconds = delayNs / NS_PER_SECOND;
    int nanoseconds = delayNs % NS_PER_SECOND;
    struct timespec reqDelay = {seconds, nanoseconds};
    nanosleep(&reqDelay, (struct timespec *) NULL);
}

int runCommand(const char* command)
{
    // Execute the shell command (output into pipe)
    FILE *pipe = popen(command, "r");
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
    // if (exitCode != COMMAND_SUCCESS) {
    //     SYS_WARN("Command '%s' failed with exit code '%d'.\n", command, exitCode);
    // }
    return exitCode;
}

int overwriteFile(const char* filePath, const char* string, bool exitOnFailure)
{
    assert(filePath != NULL);
    assert(string != NULL);

    FILE* pFile = fopen(filePath, "w");
    if (pFile == NULL) {
        // FILE_OPEN_ERR(filePath, exitOnFailure);
        return ERR_OPEN;
    }
    int res = fprintf(pFile, string);
    if (res <= 0) {
        // FILE_WRITE_ERR(filePath, exitOnFailure);
        return ERR_WRITE;
    }

    if (fclose(pFile)) {
        // FILE_CLOSE_ERR(filePath, exitOnFailure);
        return ERR_CLOSE;
    }

    return OK;
}

int readFile(char* filePath, void* outData, size_t numBytesPerItem, size_t numItems, bool exitOnFailure)
{
    FILE* pFile = fopen(filePath, "r");
    if (pFile == NULL) {
        // if (exitOnFailure) {
        //     FILE_OPEN_ERR(filePath, exitOnFailure);
        // }
        return ERR_OPEN;
    }

    size_t res = fread(outData, numBytesPerItem, numItems, pFile);

    if (res < numItems) {
        // FILE_READ_ERR(filePath, exitOnFailure);
        return ERR_READ;
    }

    if (fclose(pFile)) {
        // FILE_CLOSE_ERR(filePath, exitOnFailure);
        return ERR_CLOSE;
    }

    return OK;
}