// Modified code provided By Brian Fraser, Modified from Linux Programming Unleashed (book)

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>			// for strncmp()
#include <unistd.h>			// for close()
#include <ctype.h>
#include <string>

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
pthread_mutex_t waitMutex = PTHREAD_MUTEX_INITIALIZER;

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
	while (1) {
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

		// Set the command to be the last valid command if the command is the new line character
		printf("message received (%d bytes): \n\n'%s'\n", bytesRx, messageRx);

		// Checks if the command matches any valid commands and returns an unknown command message if not
		if (strncmp(messageRx, "stop\n", strlen("stop\n")) == 0) {
			char messageTx[MSG_MAX_LEN];
			sprintf(messageTx, "Program terminating.\n");

			sin_len = sizeof(sinRemote);
			sendto( socketDescriptor,
				messageTx, strlen(messageTx),
				0,
				(struct sockaddr *) &sinRemote, sin_len);
			pthread_mutex_unlock(&waitMutex);
			break;
		}
		else if (strncmp(messageRx, "modeNone", strlen("modeNone")) == 0) {
			char messageTx[MSG_MAX_LEN];
			sprintf(messageTx, "Program terminating.\n");

			sin_len = sizeof(sinRemote);
			sendto( socketDescriptor,
				messageTx, strlen(messageTx),
				0,
				(struct sockaddr *) &sinRemote, sin_len);
			pthread_mutex_unlock(&waitMutex);
			break;
		}
		else if (strncmp(messageRx, "update fields", strlen("update fields")) == 0) {
			char messageTx[MSG_MAX_LEN];
			sprintf(messageTx, "INSERT FIELD DATA\n");

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
			printf("%s\n", str);

			char messageTx[MSG_MAX_LEN];
			sprintf(messageTx, "%s", str);

			sin_len = sizeof(sinRemote);
			sendto( socketDescriptor,
				messageTx, strlen(messageTx),
				0,
				(struct sockaddr *) &sinRemote, sin_len);
			getUpTime();
		}
		else if (strncmp(messageRx, "help\n", strlen("help\n")) == 0) {
			char messageTx[MSG_MAX_LEN];
			sprintf(messageTx, "Accepted command examples:\n"
				"count		-- display total number of samples taken.\n"
				"length		-- display number of samples in history (both max, and current).\n"
				"history		-- display the full sample history being saved.\n"
				"get 10		-- display the 10 most recent history values.\n"
				"dips		-- display number of dips in history\n"
				"stop		-- cause the server program to end.\n"
				"<enter>		-- repeat last command\n");

			sin_len = sizeof(sinRemote);
			sendto( socketDescriptor,
				messageTx, strlen(messageTx),
				0,
				(struct sockaddr *) &sinRemote, sin_len);
		}
		else {
			char messageTx[MSG_MAX_LEN];
			sprintf(messageTx, "Unknown command. Type 'help' for command list.\n");

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

void UdpServer_initialize(void)
{
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