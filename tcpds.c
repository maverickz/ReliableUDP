/* tcpds.c using UDP */

/* Server for accepting an Internet stream connection on port 1040 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "tcpheader.c"
#include "crc32.c"
#define MESSAGE_SIZE 1000
#define SOCK_ADDR_SIZE 16
#define TCP_HEADER_SIZE 20
#define FTPS_HOST "127.0.0.1"
#define FTPS_PORT "9002"
#define TCPDS_PORT "9001"
#define ACK_PORT "9023"
#define TROLL_PORT "12000"

struct tcpheader tcpHeader;
struct tcpheader ackPacket;

struct {
	struct sockaddr_in header;
	char body[TCP_HEADER_SIZE + MESSAGE_SIZE];
} message;


main(int argc,char *argv[])
{
	struct sockaddr_in sin_addr,from,troll,destination, ack_addr; /* structure for socket name setup */
	struct sockaddr_in name;
	char messageBuffer[SOCK_ADDR_SIZE + TCP_HEADER_SIZE + MESSAGE_SIZE];               /* buffer for holding read data */
	char tcpdsBuffer[1000];
	char ackBuffer[20];
	struct hostent *hp, *gethostbyname();
	int sock;
    int ftpsSocket;
	int fromTrollSock;
	int toTrollSock;
	int ackSocket;
	int fromLen;                   
	int received = 0;
	int bytesReceived = 0;
	int bytesSent = 0;
	int fileSize;
	int fileName[20]; // stores filename
	int sent = 0;
	int ackSent = 0;
	unsigned short int crc32;
	unsigned short int computedcrc32;
	unsigned short int ackcrc32;
	unsigned int seqNumber;
	FILE *fp;
	
	fp  = fopen("checksum_op.txt","wb");
  
	printf("tcpds waiting for remote connection fromm clients... \n");

	gen_crc_table();
	/*initialize socket connection in unix domain*/
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock < 0) {
		perror("opening datagram socket");
		exit(1);
	}
	
	fromTrollSock = socket(AF_INET, SOCK_DGRAM, 0);
	if(fromTrollSock < 0) {
		perror("Error opening datagram socket: fromTrollSock");
		exit(1);
	}
	
	ftpsSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if(ftpsSocket < 0) {
		perror("Error opening datagram socket: ftpsSocket");
		exit(1);
	}
	
	ackSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if(ackSocket < 0) {
		perror("Error opening datagram socket: ackSocket");
		exit(1);
	}

	/* construct name of socket to send to */
	sin_addr.sin_family = AF_INET;
	sin_addr.sin_addr.s_addr = INADDR_ANY;
	sin_addr.sin_port = htons(atoi(TCPDS_PORT));
	
	/* bind socket name to socket */
	if(bind(fromTrollSock, (struct sockaddr *)&sin_addr, sizeof(sin_addr)) < 0) {
		perror("Error binding datagram socket");
		exit(2);
	}

	destination.sin_family = AF_INET;
	destination.sin_addr.s_addr = inet_addr(FTPS_HOST);
	destination.sin_port = htons(atoi(FTPS_PORT));
	
	hp = gethostbyname(argv[1]);
	if(hp == 0) {
		fprintf(stderr, "%s: unknown host\n", argv[1]);
		exit(2);
	}

	/* construct name of socket to send to */
	bcopy((void *)hp->h_addr, (void *)&ack_addr.sin_addr, hp->h_length);
	ack_addr.sin_family = AF_INET;
	ack_addr.sin_port = htons(atoi(ACK_PORT));
	
	toTrollSock = socket(AF_INET, SOCK_DGRAM,0);
	if(toTrollSock < 0) {
		perror("opening datagram socket");
		exit(2);
	}

	/* construct name for connecting to troll */
	name.sin_family = AF_INET;
	name.sin_port = htons(atoi(TROLL_PORT));
	name.sin_addr.s_addr= INADDR_ANY;

	if(bind(toTrollSock, (struct sockaddr *)&name, sizeof(name)) < 0) {
		perror("getting socket name");
		exit(2);
	}
	
	message.messageHeader.sin_family = htons(AF_INET);
	message.messageHeader.sin_port = htons(atoi(TCPDC_PORT));

	printf("TCPD Server waiting on port # %d\n", ntohs(name.sin_port));
	
	/* Clear buffer */
	bzero(messageBuffer,SOCK_ADDR_SIZE + TCP_HEADER_SIZE + MESSAGE_SIZE);
	fromLen = sizeof(struct sockaddr_in);
	
	received = recvfrom(fromTrollSock, messageBuffer, SOCK_ADDR_SIZE + TCP_HEADER_SIZE + MESSAGE_SIZE, 0, (struct sockaddr *)&from, &fromLen);
	//usleep(10000);
	/*TCP header processing*/
	memcpy(&tcpHeader,messageBuffer+SOCK_ADDR_SIZE,sizeof(tcpHeader));
	crc32 = tcpHeader.checkSum;
	
	tcpHeader.checkSum = 0;
	memcpy(messageBuffer+SOCK_ADDR_SIZE,&tcpHeader,sizeof(tcpHeader));
	computedcrc32 = update_crc(messageBuffer+SOCK_ADDR_SIZE, received-16);
	
	if(crc32 != computedcrc32) {	
		printf("Checksum mis-match :\n");
		printf("Checksum: %08x\n",crc32);
		printf("Computed Checksum: %08x\n",computedcrc32);
		fprintf (fp, "Checksum mis-match Packet : %d \n",tcpHeader.sequenceNumber);
	} else {
		bytesReceived += received - (SOCK_ADDR_SIZE + TCP_HEADER_SIZE) - 24;
		printf("BytesReceived: %d\n",bytesReceived);
	}
	
	seqNumber = tcpHeader.sequenceNumber;
	printf("Sending ackPacket for packet with seq num: %d\n",seqNumber);
	bzero(ackPacket,TCP_HEADER_SIZE);
	ackPacket.sequenceNumber = seqNumber;
	memcpy(ackBuffer,&ackPacket,TCP_HEADER_SIZE);
	ackPacket.checkSum = update_crc(ackBuffer,TCP_HEADER_SIZE);
	memcpy(message.messageBody,&ackPacket,TCP_HEADER_SIZE);
	
	
	memcpy(tcpdsBuffer,messageBuffer + (SOCK_ADDR_SIZE + TCP_HEADER_SIZE),sizeof(tcpdsBuffer));
	memcpy(&fileSize,tcpdsBuffer,4);
	memcpy(fileName,tcpdsBuffer+4,20);
	printf("FileSize= %d   FileName= %s\n",fileSize,fileName);
	
	sent = sendto(ftpsSocket, tcpdsBuffer, 1000, 0, (struct sockaddr *)&destination, sizeof destination);
	bytesSent += sent - 24;
	
	//ackSent = sendto(ackSocket, ackBuffer, 20, 0, (struct sockaddr *)&ack_addr, sizeof ack_addr);
	sendto(toTrollSock, ackBuffer, 20, 0, (struct sockaddr *)&ack_addr, sizeof ack_addr);
	
	//usleep(10000);
	
	bzero(tcpdsBuffer,1000);
	bzero(messageBuffer,1036);
	
	printf("bytesSent: %d,fileSize: %d \n",bytesSent, fileSize);
	/*read and send the entire file*/
	while(bytesSent < fileSize){
		received = recvfrom(fromTrollSock, messageBuffer, SOCK_ADDR_SIZE + TCP_HEADER_SIZE + MESSAGE_SIZE, 0, (struct sockaddr *)&from, &fromLen);
		bytesReceived += received - (SOCK_ADDR_SIZE + TCP_HEADER_SIZE);
		memcpy(tcpdsBuffer,messageBuffer + (SOCK_ADDR_SIZE + TCP_HEADER_SIZE),sizeof(tcpdsBuffer));
		memcpy(&tcpHeader,messageBuffer+SOCK_ADDR_SIZE,sizeof(tcpHeader));
		printf("Checksum: %08x\n",tcpHeader.checkSum);
		crc32 = tcpHeader.checkSum;
		tcpHeader.checkSum = 0;
		memcpy(messageBuffer+SOCK_ADDR_SIZE,&tcpHeader,sizeof(tcpHeader));
		computedcrc32 = update_crc(messageBuffer+SOCK_ADDR_SIZE, received-16);
		if(crc32 != computedcrc32) {	
			printf("Checksum mis-match :\n");
			printf("Checksum: %08x\n",crc32);
			printf("Computed Checksum: %08x\n",computedcrc32);
			fprintf (fp, "Checksum mis-match Packet : %d \n",tcpHeader.sequenceNumber);
		}
		tcpHeader.sequenceNumber = ++seqNumber;
		memcpy(ackBuffer,&tcpHeader,20);
		printf("Received : %d\n",tcpHeader.sequenceNumber);
		usleep(10000);
		ackSent = sendto(ackSocket, ackBuffer, 20, 0, (struct sockaddr *)&ack_addr, sizeof ack_addr);
		sent = sendto(ftpsSocket, tcpdsBuffer, received -(SOCK_ADDR_SIZE + TCP_HEADER_SIZE), 0, (struct sockaddr *)&destination, sizeof destination);
		bytesSent += sent;
	}
	
	printf("\nTotal bytes received: %d\n",bytesReceived);
	/* close all connections and remove socket file */
	close(ftpsSocket);
	close(fromTrollSock);
}
