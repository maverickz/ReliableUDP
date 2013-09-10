/* ftps.c using UDP */

/* Server for accepting an Internet stream connection on a given port */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#define OUTPUT_FILE_PATH "/home/0/saanthal/678/HW2/output"
struct sockaddr_in from;
int fromLen;

main(int argc,char *argv[])
{
	int sock;                     /* initial socket descriptor */
	struct sockaddr_in sin_addr; /* structure for socket name setup */
	char serverBuffer[1000];          /* buffer for holding read data */
	int received = 0;
	int bytesReceived = 0;
	int fileSize;
	int fileName[20]; // stores filename

	FILE *pFile;//filepointer

	printf("ftps waiting for remote connection from clients ...\n");

	/*initialize socket connection in unix domain*/

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock < 0) {
		perror("Error opening datagram socket");
		exit(1);
	}

	/* construct name of socket to send to */
	sin_addr.sin_family = AF_INET;
	sin_addr.sin_addr.s_addr = INADDR_ANY;
	sin_addr.sin_port = htons(atoi(argv[1]));

	/* bind socket name to socket */
	if(bind(sock, (struct sockaddr *)&sin_addr, sizeof(struct sockaddr_in)) < 0) {
		perror("Error binding stream socket");
		exit(2);
	}
	pFile=fopen("OUTPUT_FILE_PATH.jpg","wb");

	/* put all zeros in buffer (clear) */
	bzero(serverBuffer,1000);

	//extract filesize and filename from the first 24 bytes of the 1000 bytes

	fromLen = sizeof(struct sockaddr_in);
	received = RECV(sock, serverBuffer, 1000, 0);
	memcpy(&fileSize,serverBuffer,4);
	memcpy(fileName,serverBuffer+4,20);
	printf("File Size: %d   File Name: %s",fileSize,fileName);
	
	fwrite(serverBuffer+24,1,976,pFile);
	fflush(pFile);
	bytesReceived += received - 24;

	/* read from msgsock and place in serverBuffer and also write to the file*/

	while(bytesReceived < fileSize){
		received = RECV(sock, serverBuffer, 1000, 0);
		bytesReceived += received;
		fwrite(serverBuffer, sizeof(char), received, pFile);
		fflush(pFile);
	}

	/* close all connections and remove socket file */
	fclose(pFile);	
	close(sock);
}


int RECV(int sock, char *buffer, int bufferSize, int flags)
{
	int bytesReceived;
	bytesReceived = recvfrom(sock, buffer, bufferSize, flags, (struct sockaddr *) &from, &fromLen);
	if(bytesReceived < 0){
		perror("Error receiving datagram message");
		exit(3);
	}
	return(bytesReceived);
}

