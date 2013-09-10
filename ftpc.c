/* Example: client.c sending and receiving datagrams using UDP */ 
#include <netdb.h> 
#include <stdio.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <time.h>
#include <unistd.h>
#define MAX_MES_LEN 1000 
#define TCPDC_PORT "1112"
#define TCPDC_HOST "127.0.0.1"

struct sockaddr_in sin_addr, tcpdc_ftpc_send;

/* client program called with host name and port number of server */ 
main(int argc, char *argv[])
{
  char flagBuffer[4];
  int tcpdc_ftpc, received;
  int flag, fromLen;
  char clientBuffer[MAX_MES_LEN];
	char *fileName = argv[2];
	char remoteHostName[20];
	char *filePath = "/home/0/saanthal/678/HW2/1.jpg";
    struct hostent *hp, *gethostbyname();
	FILE *pFile;
	size_t result;
	int remoteHost;
	int sock;
	int read = 0;
    int bytesRead = 0;				/*No of bytes read from the file*/
    int sent;					/*No of bytes sent over the socket in each send() call*/
    long bytesSent = 0;			/*Total no of bytes transferred across the socket*/
	int fileSize=0;
	
		
    if(argc < 3) {
		printf("Usage: ftpc <ftps_host_name> <file_name>\n");
		exit(1);
    }

	printf("Client connected to tcp daemon\n");
	
    /* create socket for connecting to tcpd */	
    sock = socket(AF_INET, SOCK_DGRAM,0);
    if(sock < 0) {
		perror("Error opening datagram socket");
		exit(2);
    }

    tcpdc_ftpc = socket(AF_INET, SOCK_DGRAM, 0);
    if(tcpdc_ftpc < 0){
      perror("Error opening datagram socket tcpdc_ftpc\n");
      exit(2);
    }

    tcpdc_ftpc_send.sin_family = AF_INET;
    tcpdc_ftpc_send.sin_port = htons(7301);
    tcpdc_ftpc_send.sin_addr.s_addr = inet_addr(TCPDC_HOST);

	if (bind(tcpdc_ftpc, (struct sockaddr *)&tcpdc_ftpc_send, sizeof tcpdc_ftpc_send) < 0) {
		perror("client bind");
		exit(1);
	}

    /* construct name for connecting to server */
    sin_addr.sin_family = AF_INET;
    sin_addr.sin_port = htons(atoi(TCPDC_PORT));
	sin_addr.sin_addr.s_addr = inet_addr(TCPDC_HOST);
	
	
	pFile=fopen(fileName,"rb");
	if (pFile==NULL) {
		fprintf(stderr, "Error Opening %s\n", fileName); 
		exit(4);
	}
	
	fileSize = sizeOfFile(pFile);
	printf("Filesize %d\n",fileSize);

	fromLen = sizeof(struct sockaddr_in);
	
	/* write file size, file name and the first 976 bytes from file to sock */
	strcpy(remoteHostName, argv[1]);
	SEND(sock, remoteHostName, 20, 0, &sin_addr, sizeof(sin_addr));
	memcpy(clientBuffer,&fileSize,4);
	memcpy(clientBuffer+4,fileName,20);
	read = fread(clientBuffer+24, sizeof(char), sizeof(clientBuffer) - 24, pFile);
	bytesRead += read;
	printf("Waiting on send \n");
	sent = SEND(sock, clientBuffer, 1000, 0, &sin_addr, sizeof(sin_addr));
	bytesSent += sent - 24;
	
	
	printf("ftpc sent FileSize: %d, FileName: %s\n",fileSize, fileName);
	
	/*read and send the entire file*/
	while(bytesSent < fileSize){
	  printf("\n inside while");
	  received = recvfrom(tcpdc_ftpc, &flagBuffer, 4, 0, (struct sockaddr *)&tcpdc_ftpc_send, &fromLen);
	  printf("%d bytes received\n", received);
	  memcpy(&flag, flagBuffer, 4);
	  printf("%d,%d",received, flag);
	  if(flag == 1){
		read = fread(clientBuffer, 1, 1000, pFile);
		bytesRead += read;
		sent = SEND(sock, clientBuffer, read, 0);
		usleep(300000);
		bytesSent += sent;
		flag = 0;
		printf("Bytes sent: %d\n",bytesSent);
	  }
	}

	printf("Total bytes read:=%d\n Total bytes sent=%d\n",bytesRead,bytesSent);	
	close(sock);
	fclose(pFile);
}


int SEND(int sock, char *buffer, int bufferSize, int flags)
{
	socklen_t destLen = sizeof(sin_addr);
	int bytesSent = 0;
	bytesSent = sendto(sock,(const void*) buffer, bufferSize, flags, (const struct sockaddr *) &sin_addr, destLen);
	if(bytesSent < 0)
	{
		perror("Error in SEND: sending datagram message");
		exit(5);
	}
	return(bytesSent);
}


int sizeOfFile(FILE *pFile)
{
        double fileSize;
		fseek (pFile , 0 , SEEK_END);  /* Take the file pointer to the end of the file */
		fileSize = ftell (pFile);  /* Obtain file size */
		rewind (pFile);  /* Release the file pointer to the original position */
        return fileSize;
}
