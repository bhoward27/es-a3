
#include <stdio.h>
#include <stdlib.h>


#include "udpServer.h"

int main() {
    UdpServer_initialize();

    // Lock the wait mutex
	pthread_mutex_lock(&waitMutex);

    // Wait for mutex to unlock
    pthread_mutex_lock(&waitMutex);
    pthread_mutex_unlock(&waitMutex);

    printf("Cleaning everything up.\n");
    UdpServer_cleanup();
    printf("Done!\n");
}