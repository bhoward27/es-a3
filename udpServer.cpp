// Modified code provided By Brian Fraser, Modified from Linux Programming Unleashed (book)

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <netdb.h>
#include <cstring>			// for strncmp()
#include <unistd.h>			// for close()
#include <ctype.h>
#include <string>
#include <exception>

#include "udpServer.h"

#define MSG_MAX_LEN 1500
#define PORT        12345
#define READING_MAX_LEN 8
#define MAX_READINGS_PER_PACKET 210
#define MAX_SIZE_HISTORY_NUM_BUFFER 5
#define LENGTH_OF_GET_COMMAND 4
#define MAX_READINGS_PER_LINE 20
#define UPTIME_FILE "/proc/uptime"

static pthread_t samplerId;
static int socketDescriptor;
static ShutdownManager* pShutdownManager = nullptr;
static BeatPlayer* pBeatPlayer = nullptr;
static AudioMixer* pAudioMixer = nullptr;

static float getUpTime()
{
	FILE *uFile = fopen(UPTIME_FILE, "r");
    if (uFile == NULL) {
        printf("ERROR: Unable to open value file.\n");
        exit(1);
    }

    const int MAX_LENGTH = 1024;
    char buff[MAX_LENGTH];
    fgets(buff, MAX_LENGTH, uFile);
	fclose(uFile);

	// Learned how to convert string to float in c++ from this link: https://cplusplus.com/reference/string/stof/
	std::string times (buff);
	std::string::size_type sz;
	float timeInSeconds = std::stof (times,&sz);
	return timeInSeconds;
}

static void *updServerThread(void *args)
{
	while (!pShutdownManager->isShutdownRequested()) {
		// Get the data (blocking)
		// Will change sin (the address) to be the address of the client.
		// Note: sin passes information in and out of call!
		struct sockaddr_in sinRemote;
		unsigned int sin_len = sizeof(sinRemote);
		char messageRx[MSG_MAX_LEN];

		// Pass buffer size - 1 for max # bytes so room for the null (string data)
		int bytesRx = recvfrom(socketDescriptor,
			messageRx, MSG_MAX_LEN - 1, 0,
			(struct sockaddr *) &sinRemote, &sin_len);


		// Make it null terminated (so string functions work)
		// - recvfrom given max size - 1, so there is always room for the null
		messageRx[bytesRx] = 0;

		// Checks if the command matches any valid commands and returns an unknown command message if not
		if (strncmp(messageRx, "terminate", strlen("terminate")) == 0) {
			pShutdownManager->requestShutdown();
			pBeatPlayer->stop();

			char messageTx[MSG_MAX_LEN];
			sprintf(messageTx, "Program terminating.\n");

			sin_len = sizeof(sinRemote);
			sendto( socketDescriptor,
				messageTx, strlen(messageTx),
				0,
				(struct sockaddr *) &sinRemote, sin_len);
			break;
		}
		else if (strncmp(messageRx, "mode none", strlen("mode none")) == 0) {
			pBeatPlayer->stop(); // Equivalently, could call play(Beat::none).

			char messageTx[MSG_MAX_LEN];
			sprintf(messageTx, "mode none");

			sin_len = sizeof(sinRemote);
			sendto( socketDescriptor,
				messageTx, strlen(messageTx),
				0,
				(struct sockaddr *) &sinRemote, sin_len);
		}
		else if (strncmp(messageRx, "mode rock1", strlen("mode rock1")) == 0) {
			pBeatPlayer->play(Beat::standard);

			char messageTx[MSG_MAX_LEN];
			sprintf(messageTx, "mode standard");

			sin_len = sizeof(sinRemote);
			sendto( socketDescriptor,
				messageTx, strlen(messageTx),
				0,
				(struct sockaddr *) &sinRemote, sin_len);
		}
		else if (strncmp(messageRx, "mode rock2", strlen("mode rock2")) == 0) {
			pBeatPlayer->play(Beat::alternate);

			char messageTx[MSG_MAX_LEN];
			sprintf(messageTx, "mode alternate");

			sin_len = sizeof(sinRemote);
			sendto( socketDescriptor,
				messageTx, strlen(messageTx),
				0,
				(struct sockaddr *) &sinRemote, sin_len);
		}
		else if (strncmp(messageRx, "volume up", strlen("volume up")) == 0) {
			int currentVolume = pAudioMixer->increaseVolume();

			char str[1024];
			sprintf(str, "volume %d", currentVolume);

			char messageTx[MSG_MAX_LEN];
			sprintf(messageTx, "%s", str);

			sin_len = sizeof(sinRemote);
			sendto( socketDescriptor,
				messageTx, strlen(messageTx),
				0,
				(struct sockaddr *) &sinRemote, sin_len);
		}
		else if (strncmp(messageRx, "volume down", strlen("volume down")) == 0) {
			int currentVolume = pAudioMixer->decreaseVolume();

			char str[1024];
			sprintf(str, "volume %d", currentVolume);

			char messageTx[MSG_MAX_LEN];
			sprintf(messageTx, "%s", str);

			sin_len = sizeof(sinRemote);
			sendto( socketDescriptor,
				messageTx, strlen(messageTx),
				0,
				(struct sockaddr *) &sinRemote, sin_len);
		}
		else if (strncmp(messageRx, "tempo up", strlen("tempo up")) == 0) {
			int currentTempo = pBeatPlayer->increaseTempo();

			char str[1024];
			sprintf(str, "tempo %d", currentTempo);

			char messageTx[MSG_MAX_LEN];
			sprintf(messageTx, "%s", str);

			sin_len = sizeof(sinRemote);
			sendto( socketDescriptor,
				messageTx, strlen(messageTx),
				0,
				(struct sockaddr *) &sinRemote, sin_len);
		}
		else if (strncmp(messageRx, "tempo down", strlen("tempo down")) == 0) {
			int currentTempo = pBeatPlayer->decreaseTempo();

			char str[1024];
			sprintf(str, "tempo %d", currentTempo);

			char messageTx[MSG_MAX_LEN];
			sprintf(messageTx, "%s", str);

			sin_len = sizeof(sinRemote);
			sendto( socketDescriptor,
				messageTx, strlen(messageTx),
				0,
				(struct sockaddr *) &sinRemote, sin_len);
		}
		else if (strncmp(messageRx, "play hihat", strlen("play hihat")) == 0) {
			pAudioMixer->queueSound(&pAudioMixer->sound.hiHat);

			char messageTx[MSG_MAX_LEN];
			sprintf(messageTx, "Play hihat sound\n");

			sin_len = sizeof(sinRemote);
			sendto( socketDescriptor,
				messageTx, strlen(messageTx),
				0,
				(struct sockaddr *) &sinRemote, sin_len);
		}
		else if (strncmp(messageRx, "play snare", strlen("play snare")) == 0) {
			pAudioMixer->queueSound(&pAudioMixer->sound.snare);

			char messageTx[MSG_MAX_LEN];
			sprintf(messageTx, "Play snare sound\n");

			sin_len = sizeof(sinRemote);
			sendto( socketDescriptor,
				messageTx, strlen(messageTx),
				0,
				(struct sockaddr *) &sinRemote, sin_len);
		}
		else if (strncmp(messageRx, "play base", strlen("play base")) == 0) {
			pAudioMixer->queueSound(&pAudioMixer->sound.bassDrum);

			char messageTx[MSG_MAX_LEN];
			sprintf(messageTx, "Play base sound\n");

			sin_len = sizeof(sinRemote);
			sendto( socketDescriptor,
				messageTx, strlen(messageTx),
				0,
				(struct sockaddr *) &sinRemote, sin_len);
		}
		else if (strncmp(messageRx, "play cymbal", strlen("play cymbal")) == 0) {
			pAudioMixer->queueSound(&pAudioMixer->sound.cymbal);

			char messageTx[MSG_MAX_LEN];
			sprintf(messageTx, "Play cymbal sound\n");

			sin_len = sizeof(sinRemote);
			sendto( socketDescriptor,
				messageTx, strlen(messageTx),
				0,
				(struct sockaddr *) &sinRemote, sin_len);
		}
		else if (strncmp(messageRx, "play clave", strlen("play clave")) == 0) {
			pAudioMixer->queueSound(&pAudioMixer->sound.clave);

			char messageTx[MSG_MAX_LEN];
			sprintf(messageTx, "Play clave sound\n");

			sin_len = sizeof(sinRemote);
			sendto( socketDescriptor,
				messageTx, strlen(messageTx),
				0,
				(struct sockaddr *) &sinRemote, sin_len);
		}
		else if (strncmp(messageRx, "play tom-drum", strlen("play tom-drum")) == 0) {
			pAudioMixer->queueSound(&pAudioMixer->sound.tomDrum);

			char messageTx[MSG_MAX_LEN];
			sprintf(messageTx, "Play tom-drum sound\n");

			sin_len = sizeof(sinRemote);
			sendto( socketDescriptor,
				messageTx, strlen(messageTx),
				0,
				(struct sockaddr *) &sinRemote, sin_len);
		}
		else if (strncmp(messageRx, "update fields", strlen("update fields")) == 0) {
			Beat currentBeat = pBeatPlayer->getBeat();
			int currentBpm = pBeatPlayer->getBpm();
			int currentVolume = pAudioMixer->getVolume();

			char str[1024];
			sprintf(str, "update %d %d %d", (int)currentBeat, currentBpm, currentVolume);

			char messageTx[MSG_MAX_LEN];
			sprintf(messageTx, "%s", str);

			sin_len = sizeof(sinRemote);
			sendto( socketDescriptor,
				messageTx, strlen(messageTx),
				0,
				(struct sockaddr *) &sinRemote, sin_len);
		}
		else if (strncmp(messageRx, "UpTime", strlen("UpTime")) == 0) {
			float timeInSeconds = getUpTime();

			// Learned how to create a string in c++ from this link: https://stackoverflow.com/questions/10219225/c-create-string-of-text-and-variables
			char str[1024];
			sprintf(str, "Device up for: %d:%d:%d(H:M:S)", (int)timeInSeconds/3600, ((int)timeInSeconds/60)%60, (int)timeInSeconds%60);

			char messageTx[MSG_MAX_LEN];
			sprintf(messageTx, "%s", str);

			sin_len = sizeof(sinRemote);
			sendto( socketDescriptor,
				messageTx, strlen(messageTx),
				0,
				(struct sockaddr *) &sinRemote, sin_len);
			getUpTime();
		}
		else {
			char messageTx[MSG_MAX_LEN];
			sprintf(messageTx, "Unknown command\n");

			sin_len = sizeof(sinRemote);
			sendto( socketDescriptor,
				messageTx, strlen(messageTx),
				0,
				(struct sockaddr *) &sinRemote, sin_len);
		}
	}

	// Close
	close(socketDescriptor);

	return 0;
}

void UdpServer_initialize(ShutdownManager* pShutdownManagerArg, AudioMixer* pAudioMixerArg, BeatPlayer* pBeatPlayerArg)
{
    if (pShutdownManagerArg == nullptr || pAudioMixerArg == nullptr || pBeatPlayerArg == nullptr) {
        return;
    }
    pShutdownManager = pShutdownManagerArg;
    pAudioMixer = pAudioMixerArg;
    pBeatPlayer = pBeatPlayerArg;

	// Address
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;                   // Connection may be from network
	sin.sin_addr.s_addr = htonl(INADDR_ANY);    // Host to Network long
	sin.sin_port = htons(PORT);                 // Host to Network short

	// Create the socket for UDP
	socketDescriptor = socket(PF_INET, SOCK_DGRAM, 0);

	// Bind the socket to the port (PORT) that we specify
	bind (socketDescriptor, (struct sockaddr*) &sin, sizeof(sin));
    // Check for errors (-1)

    pthread_create(&samplerId, NULL, &updServerThread, NULL);
}

void UdpServer_cleanup(void)
{
    pthread_join(samplerId, NULL);
}