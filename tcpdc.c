/* tcpdc.c using UDP */

/* Server for accepting an Internet stream connection on a given port */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "tcpheader.c"
#include "crc.c"


#define MESSAGE_SIZE 1000
#define TCP_HEADER_SIZE 20
#define FILE_SIZE 4
#define FILE_NAME 20

#define TROLL_HOST "127.0.0.1"
#define TROLL_PORT "6001"
#define PORT "1112"
#define TCPDS_PORT "7002"
#define ACK_PORT "9023"

/* server program called with no argument */

struct tcpheader tcpHeader, ackPacket;

struct packet  
{  
  struct sockaddr_in msg_header;  
  char packetdata[TCP_HEADER_SIZE + MESSAGE_SIZE];   
};

struct {
	struct sockaddr_in messageHeader;
	char messageBody[TCP_HEADER_SIZE + MESSAGE_SIZE];
} message;


/* Storage Buffer which stores the 64 packets at any time */
struct packet storageBuffer[64];

void insertTCPHeader(unsigned int seqNumber,unsigned short int crc16)
{
	tcpHeader.sourcePort = 0;
	tcpHeader.destinationPort = 0;
	tcpHeader.sequenceNumber = seqNumber;
	tcpHeader.ackNumber = 0;
	tcpHeader.reserved = 0;
	tcpHeader.tcph_offset = 0;
	tcpHeader.tcph_fin = 0;
	tcpHeader.tcph_syn = 0;
	tcpHeader.tcph_rst = 0;
	tcpHeader.tcph_psh = 0;
	tcpHeader.tcph_ack = 0;
	tcpHeader.tcph_urg = 0;
	tcpHeader.tcph_res2 = 0;
	tcpHeader.window = 0;
	tcpHeader.checkSum = crc16;
	tcpHeader.urgPtr = 0;
}

main(int argc,char *argv[])
{
	struct sockaddr_in sin_addr,from,troll,dest, ack_addr, fromServer,dataToTimer,ackToTimer,fromTimer, tcpdc_ftpc_send; /* structure for socket name setup */
	struct hostent *hp, *shp, *gethostbyname();
	char tcpdcBuffer[1000];               /* buffer for holding read data */
	char ackBuffer[20];
	int i, tempFlag, store, var;
	char messageBuffer[4];
	int counterBuffer = 0;
	char remoteHostName[20];
	char dataToTimerBuffer[8];
	int ftpcSocket;
	int trollSocket;
	int ackSocket;
	int dataToTimerSocket;
	int ackToTimerSocket;
	int fromLen;                   
	int received = 0;
	int bytesReceived = 0;
	int fileSize;
	int fileName[20]; // stores filename
	int sent = 0;
	int bytesSent = 0;
	int ackReceived = 0;
	int counter = 0;
	int sentToTimer = 0;
	int databytesToTimer;
	int ackbytesToTimer;
	unsigned short int crc16;
	unsigned int seqNumber = 1000;
	FILE *fp;
	int tcpdc_ftpc;
	int flagArray[64];


	/* 0's stating that the packet has not been loaded, value of 1 indicated the packet is present in the storageBuffer and 2 indicates that the packet has been cleared from time program that is has been delivered properly*/

	for(i=0;i<64;i++)
	  flagArray[i] = -1;

	
	printf("tcpdc waiting for remote connection from ftpc ...\n");

	//gen_crc_table();
	
	tcpdc_ftpc = socket(AF_INET, SOCK_DGRAM, 0);
	if(tcpdc_ftpc < 0){
	  perror("Error opening datagram socket");
	  exit(2);
	}


	/*initialize socket connection in unix domain*/
	ftpcSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if(ftpcSocket < 0) {
		perror("Error opening datagram socket: ftpcSocket");
		exit(2);
	}
	
	/*initialize socket connection in unix domain*/
	trollSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if(trollSocket < 0) {
		perror("Error opening datagram socket: trollSocket");
		exit(3);
	}
	
	ackSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if(ackSocket < 0) {
		perror("Error opening datagram socket: ackSocket");
		exit(3);
	}
	
	dataToTimerSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if(dataToTimerSocket < 0) {
		perror("Error opening datagram socket: dataToTimerSocket");
		exit(3);
	}
	
	ackToTimerSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if(ackToTimerSocket < 0) {
		perror("Error opening datagram socket: ackToTimerSocket");
		exit(3);
	}

	/* construct name of socket to receive from ftpc */
	sin_addr.sin_family = AF_INET;
	sin_addr.sin_addr.s_addr = INADDR_ANY;
	sin_addr.sin_port = htons(atoi(PORT));

	
	/* bind socket name to socket */
	if(bind(ftpcSocket, (struct sockaddr *)&sin_addr, sizeof(sin_addr)) < 0) {
		perror("Error binding datagram socket: ftpcSocket");
		exit(4);
	}
	
		/* construct name of socket to receive from ftpc */
	ack_addr.sin_family = AF_INET;
	ack_addr.sin_addr.s_addr = INADDR_ANY;
	ack_addr.sin_port = htons(atoi(ACK_PORT));
	
	/* bind socket name to socket */
	if(bind(ackSocket, (struct sockaddr *)&ack_addr, sizeof(ack_addr)) < 0) {
		perror("Error binding datagram socket: ackSocket");
		exit(4);
	}
	
	troll.sin_family = AF_INET;
	troll.sin_addr.s_addr = inet_addr(TROLL_HOST);/* Internet address of machine running the troll */;
	troll.sin_port = htons(atoi(TROLL_PORT));

	message.messageHeader.sin_family = htons(AF_INET);
	message.messageHeader.sin_port = htons(atoi(TCPDS_PORT));


	/* initializing all buffer with header to forward to tcpds via troll */
	for(i=0;i<64;i++)
	  {
	storageBuffer[i].msg_header.sin_family = htons(AF_INET);
	storageBuffer[i].msg_header.sin_port = htons(atoi(TCPDS_PORT));
	  }


	recvfrom(ftpcSocket, remoteHostName, 20, 0, (struct sockaddr *)&from, &fromLen);
	printf("Host from ftpc: %s\n",remoteHostName);
	hp = gethostbyname(remoteHostName);
	printf("Connecting to %s through troll...\n",remoteHostName);
	if(hp == 0) {
		fprintf(stderr, "%s:unknown host\n",remoteHostName);
		exit(5);
	}
	
	bcopy((char *)hp->h_addr, (char *)&message.messageHeader.sin_addr, hp->h_length);
	
	/* put all zeros in buffer (clear) */
	bzero(tcpdcBuffer,1000);

	fromLen = sizeof(struct sockaddr_in);
	
	received = recvfrom(ftpcSocket, tcpdcBuffer, 1000, 0, (struct sockaddr *)&from, &fromLen);

	if(received < 0)
		printf("Error in recvfrom()\n");
	bytesReceived += received - 24;
	//memcpy(remoteHostName,tcpdcBuffer,20);
	
	// timer address creation
	dataToTimer.sin_family = AF_INET;
	dataToTimer.sin_addr.s_addr = inet_addr("127.0.0.1");
	dataToTimer.sin_port = htons(2222);
	
	// timer address creation
	ackToTimer.sin_family = AF_INET;
	ackToTimer.sin_addr.s_addr = inet_addr("127.0.0.1");
	ackToTimer.sin_port = htons(3333);
	
	// timer address creation
	fromTimer.sin_family = AF_INET;
	fromTimer.sin_addr.s_addr = inet_addr("127.0.0.1");
	fromTimer.sin_port = htons(4444);
	

	memcpy(&fileSize,tcpdcBuffer,4);
	memcpy(fileName,tcpdcBuffer+4,20);
	insertTCPHeader(seqNumber,0);
	memcpy(message.messageBody,&tcpHeader,sizeof(tcpHeader));	
	memcpy(message.messageBody+20,tcpdcBuffer,sizeof(tcpdcBuffer));	
	memcpy(fileName,message.messageBody+24,20);
	printf("%s\n", fileName);

	crc16 = update_crc(message.messageBody, received+20);
	tcpHeader.checkSum = crc16;
	printf("Checksum: %08x %08x\n",tcpHeader.checkSum, crc16);
	memcpy(message.messageBody,&tcpHeader,sizeof(tcpHeader));	
	
	for(i=0;i<(TCP_HEADER_SIZE+MESSAGE_SIZE);i++)
	  storageBuffer[0].packetdata[i] = message.messageBody[i];
	memcpy(&flagArray[0],message.messageBody,4);
	
	counterBuffer = counterBuffer + 1;
	
	sent = sendto(trollSocket, (char *)&message, sizeof message, 0, (struct sockaddr *)&troll, sizeof troll);
	
	usleep(10000);
	bytesSent += sent - (FILE_SIZE + FILE_NAME) - sizeof(struct sockaddr_in) - sizeof(tcpHeader);
	fp=fopen("tcpdc_output","wb");
	fwrite(tcpdcBuffer + 24,1,976,fp);
	fflush(fp);
	bzero(tcpdcBuffer,1000);
	memcpy(dataToTimerBuffer,&seqNumber,4);
	seqNumber++;
	counter++;
	memcpy(dataToTimerBuffer+4,&counter,4);
	databytesToTimer = sendto(dataToTimerSocket, dataToTimerBuffer, sizeof dataToTimerBuffer, 0, (struct sockaddr *)&dataToTimer, sizeof dataToTimer);
	printf("Data to timer: %d\n",databytesToTimer);
	usleep(10000);
	
	tcpdc_ftpc_send.sin_family = AF_INET; 
        tcpdc_ftpc_send.sin_port= htons(7301); 
        tcpdc_ftpc_send.sin_addr.s_addr = inet_addr("127.0.0.1");

	ackReceived = recvfrom(ackSocket, ackBuffer, 20, 0, (struct sockaddr *)&fromServer, &fromLen);
	
	ackbytesToTimer = sendto(ackToTimerSocket, ackBuffer, sizeof ackBuffer, 0, (struct sockaddr *)&ackToTimer, sizeof ackToTimer);
	printf("Ack to timer: %d\n",ackbytesToTimer);
	memcpy(&ackPacket,ackBuffer,20);
	printf("Bytes received: %d\n",ackReceived);
	printf("Ack : %d\n", ackPacket.sequenceNumber);
	usleep(10000);	
	
	printf("before while\n");
	/*read and send the remaining file*/
	
	while(bytesReceived < fileSize){
	  // printf("\ninside while\n");
	  if(counterBuffer < 64)
	    {
	   printf("inside countbuffer<64\n"); 
		  tempFlag = 1;
	      memcpy(messageBuffer, &tempFlag, 4);
	      var = sendto(tcpdc_ftpc, (char *)&messageBuffer, 4, 0, (struct sockaddr *)&tcpdc_ftpc_send, sizeof tcpdc_ftpc_send);
		  memcpy( &tempFlag,messageBuffer, 4);
	     printf("%d %d\n",var, tempFlag); 
	    }
	  else
	    {
	      tempFlag = 0;
	      memcpy(messageBuffer, &tempFlag, 4);
	      sendto(tcpdc_ftpc, (char *)&messageBuffer, 4, 0, (struct sockaddr *)&tcpdc_ftpc_send, sizeof tcpdc_ftpc_send);

	    }
	 
	if(tempFlag == 1)
		{
		printf("tempFlag=1\n");	 
		received = recvfrom(ftpcSocket, tcpdcBuffer, 1000, 0, (struct sockaddr *)&from, &fromLen);
		printf("%d bytes received from ftpc\n",received);
		bytesReceived += received;
		memcpy(message.messageBody+20,tcpdcBuffer,received);
		fwrite(tcpdcBuffer,1,received,fp);
		fflush(fp);		
		insertTCPHeader(seqNumber,0);
		memcpy(message.messageBody,&tcpHeader,sizeof(tcpHeader));
		crc16 = update_crc(message.messageBody, received+20);
		tcpHeader.checkSum = crc16;
		
		memcpy(message.messageBody,&tcpHeader,sizeof(tcpHeader));
		
		for(i=0;i<64;i++)
		  {
		  if(flagArray[i] == -1)
		    {
		      store = i;
		      break;
		    }
		  }

		for(i=0;i<(TCP_HEADER_SIZE+MESSAGE_SIZE);i++)
		  storageBuffer[store].packetdata[i] = message.messageBody[i];
		memcpy(&flagArray[store],message.messageBody,4);
		counterBuffer++;
		
		sent = sendto(trollSocket, (char *)&message, received + sizeof(struct sockaddr_in) + sizeof(struct tcpheader), 0, (struct sockaddr *)&troll, sizeof troll);
		bytesSent += sent - sizeof(struct sockaddr_in) - sizeof(struct tcpheader);	
		printf("%d bytes sent to troll\n",sent);

		/*Send data to timer to be inserted into delta list*/
		memcpy(dataToTimerBuffer,&seqNumber,4);
		seqNumber++;
		counter++;
		memcpy(dataToTimerBuffer+4,&counter,4);
		databytesToTimer = sendto(dataToTimerSocket, dataToTimerBuffer, sizeof dataToTimerBuffer, 0, (struct sockaddr *)&dataToTimer, sizeof dataToTimer);
		printf("%d bytes data sent to timer\n",databytesToTimer);
		usleep(10000);
	}	
		


/* Details any is received from the timer */

		ackReceived = recvfrom(ackSocket, ackBuffer, 20, 0, (struct sockaddr *)&fromServer, &fromLen);
		memcpy(&ackPacket,ackBuffer,20);
		printf("%d bytes ack received from tcpds\n",ackReceived);


/* Details any is sent to the timer, from tcpds via tcpdc */ 
		
		ackbytesToTimer = sendto(ackToTimerSocket, ackBuffer, sizeof ackBuffer, 0, (struct sockaddr *)&ackToTimer, sizeof ackToTimer);
		printf("%d bytes ack sent to timer\n",ackbytesToTimer);
		//printf("Ack : %d\n", ackPacket.sequenceNumber);

/* Checking whether the any data received from the timer code, if so takes that sequence number from the buffer and asks ftpc to send new data */
	
		if(ackReceived != 0)
		  {
		    counterBuffer--;
		    
		    if(counterBuffer < 0)
		      counterBuffer = 0;
		    
		    for(i=0;i<64;i++)
		      {
			if(flagArray[i] == ackPacket.sequenceNumber)
			  flagArray[i] = -1;
		      }
		  }
		
	}
	printf("File size: %d\nTotal bytes read: %d\nTotal bytes sent: %d\n",fileSize,bytesReceived,bytesSent);	

	

	/* close all connections and remove socket file */
	close(ftpcSocket);
	close(trollSocket);
}

