#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "main.h"
#include "aes.h"

int main(int argc, const char *argv[])
{
	int sockfd, n, i;
	uint8_t cmd, buffer[BLOCK_SIZE] = {0};
	int32_t nonce;
	struct sockaddr_in serv_addr;
	struct timeval timeout;
	FILE *rng = 0;

	//Decode argument
	if (argc != 2)
	{
		printf("Usage:\tghetto_bmc [COMMAND]"
			"\nList of commands:"
			"\n\tSTATUS - Query the current status of the server"
			"\n\tSTART - Turn on the server"
			"\n\tSHUTDOWN - Turn off the server"
			"\n\tREBOOT - Restart the server"
			"\n\nWARNING: Use REBOOT and SHUTDOWN only as a last resort "
			"when the server is unresponsive! The recomended way to halt/reboot "
			"the server is through SSH.\n");
		return -1;
	}
	if (!strcmp(argv[1], "STATUS"))
	{
		cmd = STATUS;
	}
	else if (!strcmp(argv[1], "START"))
	{
		cmd = START;
	}
	else if (!strcmp(argv[1], "SHUTDOWN"))
	{
		cmd = SHUTDOWN;
	}
	else if (!strcmp(argv[1], "REBOOT"))
	{
		cmd = REBOOT;
	}
	else
	{
		printf("ERROR: Unrecognised command."
			"\nCall without parameters to view the help.\n");
		return -2;
	}

	//Initialise the AES key
	struct AES_ctx enc_ctx;
	AES_init_ctx(&enc_ctx, (const uint8_t*) aes_key);
	//Open a socket
	sockfd = socket(AF_INET, SOCK_PROTOCOL, 0);
	if (sockfd < 0)
	{
		perror("ERROR opening socket\n");
		return -3;
	}
	memset(&serv_addr, 0, sizeof(serv_addr));
	//Set the server address and port
	serv_addr.sin_family = AF_INET;
	if (!(inet_aton(server_addr, (struct in_addr *) &serv_addr.sin_addr.s_addr)))
	{
		perror("ERROR converting server address\n");
		close(sockfd);
		return -4;
	}
	serv_addr.sin_port = htons(SERVER_PORT);
	//Set timeout to 30 seconds
	timeout.tv_sec = 30;
	timeout.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*) &timeout, sizeof timeout);
	//Connect to the server
	printf("Attempting to connect...\n");
	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("ERROR connecting\n");
		close(sockfd);
		return -5;
	}

	//Request nonce
	buffer[0] = 1;
	n = write(sockfd, buffer, 1);
	if (n < 0)
	{
		perror("ERROR writing to socket\n");
		close(sockfd);
		return -6;
	}
	errno = 0;
	read(sockfd, buffer, BLOCK_SIZE);
	if (errno)
	{
		perror("ERROR: Connection timed out\n");
		close(sockfd);
		return -7;
	}
	AES_ECB_decrypt(&enc_ctx, buffer);
	if (buffer[0] == NONCE_PACKET)
	{
		nonce = *(int32_t*) (buffer + 1);
		printf("Nonce is %d.\n", nonce);
	}
	else
	{
		printf("ERROR: Received wrong packet type\n");
		close(sockfd);
		return -8;
	}

	//Get seed for the random padding
	rng = fopen("/dev/urandom", "r");
	if (!rng)
	{
		perror("ERROR opening /dev/random\n");
		close(sockfd);
		return -9;
	}
	if (!fread(&n, sizeof n, 1, rng))
	{
		perror("ERROR reading from /dev/urandom\n");
		close(sockfd);
		return -10;
	}
	fclose(rng);
	srandom(n);

	//Send command
	buffer[0] = COMMAND_PACKET;
	buffer[1] = cmd;
	*(int32_t*) (buffer + 2) = nonce;
	for (i = 6; i < BLOCK_SIZE; i += 2)
	{
		*(int16_t*) (buffer + i) = (int16_t) random();
	}
	AES_ECB_encrypt(&enc_ctx, buffer);
	n = write(sockfd, buffer, BLOCK_SIZE);

	//Get response
	errno = 0;
	read(sockfd, buffer, BLOCK_SIZE);
	if (errno)
	{
		perror("ERROR: Connection timed out\n");
		close(sockfd);
		return -7;
	}
	AES_ECB_decrypt(&enc_ctx, buffer);

	//Decode response
	if (*(int32_t*) (buffer + 1) != nonce)
	{
		printf("ERROR: Received wrong nonce in response\n");
		close(sockfd);
		return -11;
	}
	else if (buffer[0] == SUCCESS_PACKET)
	{
		switch (buffer[5])
		{
			case CMD_EXECUTED:
				printf("Command executed successfuly.\n");
				break;
			case PWR_ON:
				printf("Server is turned ON.\n");
				break;
			case PWR_OFF:
				printf("Server is turned OFF.\n");
				break;
			default:
				printf("ERROR: Received unknown response code\n");
				close(sockfd);
				return -12;
		}
	}
	else if (buffer[0] == ERROR_PACKET)
	{
		switch (buffer[5])
		{
			case INVALID_CMD:
				printf("Received error code - invalid command\n");
				close(sockfd);
				return -14;
			case WRONG_PKT_TYPE:
				printf("Received error code - wrong packet type\n");
				close(sockfd);
				return -15;
			case WRONG_NONCE:
				printf("Received error code - wrong nonce\n");
				close(sockfd);
				return -16;
			default:
				printf("ERROR: Received unknown error code\n");
				close(sockfd);
				return -13;
		}
	}
	else
	{
		printf("ERROR: Received wrong packet type\n");
		close(sockfd);
		return -8;
	}
	shutdown(sockfd, SHUT_WR);
	close(sockfd);
	return 0;
}
