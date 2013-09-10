#include "checksum.h"
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>





main(int argc, char *argv[])
{
	int localsock; // communication with tcpd client
	int	controlsock; // control msgs form tcpd client
	int filesize;
	fd_set read_fds; // select variable
	char fsize[4]; 
	char filename[20];
	
	struct packet sending_packet;
	struct packet received_packet;
	
	struct sockaddr_in name; // struct for local sock
	struct sockaddr_in controlname; // struct for control sock
	//struct hostent *hp, *gethostbyname();
	FILE *sourcefile; 
	int total_bytes_written,bytes_read; 
	sending_packet.packetdata.sequence_num=0; 
	

	localsock = socket(AF_INET, SOCK_DGRAM,0);
	if(localsock < 0) {
		perror("opening datagram socket");
		exit(2);
	}

		
	if(argc != 2)
	{
		printf(" \n EXPECTED WAY is ./ftpc <filename> \n");
		exit(1);
	}

	

	controlsock = socket(AF_INET, SOCK_DGRAM,0);
	if(controlsock < 0) {
		perror("opening datagram socket");
		exit(2);
	}
	controlname.sin_family = AF_INET;
	controlname.sin_port = htons(22000);
	controlname.sin_addr.s_addr = 0;
	
	if(bind(controlsock, (struct sockaddr *)&controlname, sizeof(controlname)) < 0) {
		perror("getting socket name");
		exit(2);
	}
	printf("FTPC waiting on port # %d\n", ntohs(controlname.sin_port));

	

	sourcefile = fopen(argv[1], "rb");
	if (!sourcefile)
	{
		fprintf(stderr, "Unable to open %s",argv[1]);
		return;
	}
	else
	{
		printf("FILE SUCCESSFULLY OPENED \n");
	}


	
	strcpy(filename,argv[1]);
	fseek(sourcefile, 0L, SEEK_END);
	filesize = ftell(sourcefile);
	fseek(sourcefile, 0L, SEEK_SET);
	float flsize = filesize/1024;

	
	memcpy(sending_packet.packetdata.data, &filesize, 4);
	memcpy(sending_packet.packetdata.data+4, filename, 20);


	SEND(localsock,&sending_packet,sizeof(struct packet),0);	


	FD_ZERO(&read_fds);
	FD_SET(controlsock, &read_fds);
	
	int bytesSent = 0;
	while(1)
	{	
		
		if (select(FD_SETSIZE, &read_fds, NULL, NULL, NULL) < 0 ) 
		{
			perror("Error:");
			exit(2);
		}
		
		if (FD_ISSET(controlsock, &read_fds))
		{ 	
						
			RECV_C(controlsock,&received_packet,sizeof(struct packet),0);				
			
			if(received_packet.packetdata.stop==1)
			{
				printf("\nSLEEP SIGNAL RECEIVED FROM TCPD CLIENT\n");
				usleep(10000);
			}
			
			else
			{
				
				printf("\nSEND SIGNAL FROM TCPDC\n");
				bzero(sending_packet.packetdata.data,900);
			
				
				sending_packet.packetdata.bytes_in_packet = fread(sending_packet.packetdata.data,sizeof(char),sizeof(sending_packet.packetdata.data),sourcefile);
				bytesSent += sending_packet.packetdata.bytes_in_packet;
				
				if(sending_packet.packetdata.bytes_in_packet<0)
				{
					printf("\n\n NOT ABLE TO READ THE FILE \n\n");		
					exit(1);
				}	
				
				sending_packet.packetdata.sequence_num++;
				printf("Bytes Sent: %d\n",bytesSent);
				printf("\n sequence number %d  bytes in packet %d\n",sending_packet.packetdata.sequence_num,sending_packet.packetdata.bytes_in_packet);	
				total_bytes_written= SEND(localsock,&sending_packet,sizeof(struct packet),0);		

				if(feof(sourcefile)) 
				{
					bzero(sending_packet.packetdata.data,1000);
					sending_packet.packetdata.fin=1;
					sending_packet.packetdata.bytes_in_packet=0;
					sending_packet.packetdata.sequence_num++;
					printf("\n FIN PACKET WITH SEQUENCE NUMBER %d\n",sending_packet.packetdata.sequence_num);	
					SEND(localsock,&sending_packet,sizeof(struct packet),0);			
					fclose(sourcefile);
					close(localsock);
					exit(0);
				}
				
				
			} 
		}
		
		FD_ZERO(&read_fds); 
		FD_SET(controlsock, &read_fds); 
		
	} 

}


