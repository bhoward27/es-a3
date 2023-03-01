/**
 * Provides miscellaneous functions.
 */
#ifndef UTILS_H_
#define UTILS_H_

#include "int_typedefs.h"
#include "return_val.h"

#define MEDIUM_STRING_LEN 1024

/// Sleep the calling thread for delayInMs milliseconds.
void sleepForMs(int64 delayInMs);
int runCommand(const char* command);
int overwriteFile(const char* filePath, const char* string, bool exitOnFailure);
int readFile(char* filePath, void* outData, size_t numBytesPerItem, size_t numItems, bool exitOnFailure);

#endif