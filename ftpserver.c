
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include "packet.h"

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
	int firstPacket = 1;
	struct packet receivedPacket;

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
	sin_addr.sin_port = htons(13000);

	/* bind socket name to socket */
	if(bind(sock, (struct sockaddr *)&sin_addr, sizeof(struct sockaddr_in)) < 0) {
		perror("Error binding stream socket");
		exit(2);
	}
	pFile=fopen(argv[1],"wb");
	

	/* put all zeros in buffer (clear) */
	bzero(serverBuffer,1000);



	fromLen = sizeof(struct sockaddr_in);
	
	
	/* read from msgsock and place in serverBuffer and also write to the file*/

	while(1){
		received = recvfrom(sock,(void *)&receivedPacket, sizeof(struct packet), 0, (struct sockaddr *)&sin_addr, &fromLen);
		memcpy(serverBuffer,receivedPacket.packetdata.data,receivedPacket.packetdata.bytes_in_packet);
		if(firstPacket) {
			memcpy(&fileSize,receivedPacket.packetdata.data,4);
			memcpy(fileName,receivedPacket.packetdata.data+4,20);
			printf("FileName: %s; FileSize: %d\n",fileName, fileSize);
			firstPacket = 0;
		}
			
		   if(receivedPacket.packetdata.fin==1) // if received packets fin is equal to 1 then terminate connection
			{
				printf("Fin received \n");	
				fclose(pFile);
				close(sock);
				exit(0);

			} else {
				fwrite(receivedPacket.packetdata.data, sizeof(char), receivedPacket.packetdata.bytes_in_packet, pFile);
				fflush(pFile);
			}
		
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

