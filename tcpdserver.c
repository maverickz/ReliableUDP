#include "checksum.h"
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>

#define requestport 14000


int existsflag,ackflag;
extern int errno;
int already_ackd();
int server_window[20];
int window_pointer,window_back=0,window_front=19;
int acks[64];
int packetsSentToServer[1000];
int ack_pointer=0;
struct packet server_buffer[64];
int head=0,tail=0;
int i;
int last_sent=-1;
int index_in_window;
int find_low();
int find_low_in_buffer();
void print_window();
int exists_in_window();
main(int argc, char *argv[])
{
	int trollsock,localsock, requestsocket,buflen;
	int total_bytes_sent,bytes_read;
	int bytes_recvd,bytes_written_to_file;
	int discard;
	int match;
	int exists;
	int lastackd;

	
	struct packet sending_packet;
	struct packet received_packet;
	struct packet request_packet;
	struct sockaddr_in destaddr;
	struct sockaddr_in name;
	struct sockaddr_in requestname;
	struct sockaddr_in localname;
	struct hostent *hp,*localhost,*requesthost,*request,*gethostbyname();
	
	FILE *destinationfile;
	destinationfile = fopen("tcpd.txt", "wb");
	unsigned long received_checksum;

	for(i=window_back;i<=window_front;i++)
	server_window[i]=-1;
	for(i=0;i<=63;i++)
	acks[i]=-1;

	trollsock = socket(AF_INET, SOCK_DGRAM,0);
	if(trollsock < 0) {
		perror("opening datagram socket");
		exit(2);
	}

	/* construct name for connecting to troll */
	name.sin_family = AF_INET;
	name.sin_port = htons(12000);
	name.sin_addr.s_addr= INADDR_ANY;

	/* convert hostname to IP address and enter into name */

	

	if(bind(trollsock, (struct sockaddr *)&name, sizeof(name)) < 0) {
		perror("getting socket name");
		exit(2);
	}
	int namelen=sizeof(struct sockaddr_in);

	/* Find assigned port value and print it for client to use */
	if(getsockname(trollsock, (struct sockaddr *)&name, &namelen) < 0){
		perror("getting sock name");
		exit(3);
	}
	printf("TCPD Server waiting on port # %d\n", ntohs(name.sin_port));


	requestsocket = socket(AF_INET, SOCK_DGRAM,0);
	if(localsock < 0) {
		perror("opening datagram socket");
		exit(2);
	}
	requestname.sin_family = AF_INET;
	requestname.sin_port = htons(25000);
	requesthost = gethostbyname("127.0.0.1");
	if(requesthost == 0) {
		fprintf(stderr, "LOCALHOST:unknown host\n");
		exit(3);
	}
	bcopy((char *)requesthost->h_addr, (char *)&requestname.sin_addr, requesthost->h_length);

	
	request = gethostbyname(argv[1]);
	if(request == 0) {
		fprintf(stderr, "%s:unknown host\n", argv[1]);
		exit(3);
	}	
	
	bzero ((char *)&destaddr, sizeof destaddr);
	destaddr.sin_family = htons(AF_INET);
	bcopy(request->h_addr, (char*)&destaddr.sin_addr, request->h_length);
	destaddr.sin_port = htons(requestport);	
	
	// recieve first packet with size and filename
	localsock = socket(AF_INET, SOCK_DGRAM,0);
	if(localsock < 0) {
		perror("opening datagram socket");
		exit(2);
	}
	localname.sin_family = AF_INET;
	localname.sin_port = htons(13000);
	localname.sin_addr.s_addr = INADDR_ANY;
	
	localhost = gethostbyname("127.0.0.1");
	if(localhost == 0) {
		fprintf(stderr, "%s:unknown host\n", argv[1]);
		exit(3);
	}
	
	bzero (packetsSentToServer, 1000);
	
	int firstPacket = 1;
	
	// recive other packets
	while(1)		
	{
		
		// recv data packets from troll
		recvfrom(trollsock, (void*)&server_buffer[head], sizeof(received_packet), 0, (struct sockaddr *)&name, &namelen);
		
		// Calculate Checksum
		received_checksum=crc((void *)&server_buffer[head].packetdata,sizeof(struct packetdata),0);
		
		// Compare Checksums			 
		if(received_checksum == server_buffer[head].checksum)
		{
			match=1;   // flag for checksum is 1 when checksums match
			
			ackflag=0; // flag for checking the ack array to handle duplicate packets
			for(i=0;i<64;i++)
			if(acks[i]==server_buffer[head].packetdata.sequence_num)
			{
				ackflag=1;
			}
			if(ackflag!=1) // packet is not in ack array
			{
				existsflag=0;
				// search for the packet in window
				for(i=window_back;i<=window_front;i++)
				if(server_window[i]==server_buffer[head].packetdata.sequence_num)
				{
					existsflag=1;
				}

				
				if(existsflag!=1) // does not exist in window
				{
					
					{
						index_in_window = server_buffer[head].packetdata.sequence_num % 20; 
						
						server_window[index_in_window]=server_buffer[head].packetdata.sequence_num;
						//(head++)%64; // works for small files	
						if(head<63)
						head++;
						else 
						{
							head=0;
							printf("\n BUFFER WRAPPING AROUND\n");
						}

					}
					
				}
				else if(existsflag==1) // exists in window its turn to be sent to ftps has not yet arrived
				{
					//Duplicate Packet 
					printf("\n Packet %d Already in window will be sent soon\n",server_buffer[head].packetdata.sequence_num);
				}
			}
			else if(ackflag==1) // if the ack is dropped or garbled
			{
				printf("\n Duplicate Packet Sending ACK for that packet again\n");
				//sendack for same packet
				// Duplicate Packet
				request_packet.packetdata.ack=1;
				request_packet.packetdata.ackseq=server_buffer[head].packetdata.sequence_num;
				request_packet.msg_header=destaddr;
				request_packet.checksum=crc((void *)&request_packet.packetdata,sizeof(struct packetdata),0);
				sendto(requestsocket,(void *)&request_packet, sizeof(struct packet),0, (struct sockaddr *)&requestname,sizeof(requestname));
			}	
			
		}
		else 
		{
			match=0; // checksums dont match
		}	


		if(match==1)
		{
			printf("\n\n Checksum MATCHES \n");
			// check if packet is a fin packet	
			
			int lowest_seq=find_low();
			int lowest_seq_win_index=find_low_index(lowest_seq);				
			print_window();

			if(lowest_seq==(last_sent+1)) // if the lowest sequence number in the window is the one to be sent
			{//
				// find the index of the packet with the lowest seq in the buffer
				int buffer_index=find_low_in_buffer(lowest_seq); 
				// send the packet to ftps		
				sendto(localsock,(void *)&server_buffer[buffer_index], sizeof(struct packet),0, (struct sockaddr *)&localname, sizeof(localname));
				packetsSentToServer[server_buffer[buffer_index].packetdata.sequence_num] = 1;
				memcpy(&sending_packet,&server_buffer[buffer_index],sizeof(struct packet));
				fwrite(sending_packet.packetdata.data,1,sending_packet.packetdata.bytes_in_packet,destinationfile);

				printf("sizeof(struct packet) - %d\n",sizeof(server_buffer[buffer_index]));
				printf("\n Packet with SEQ num %d SENT to FTPS\n",server_buffer[buffer_index].packetdata.sequence_num);
				if(server_buffer[buffer_index].packetdata.fin!=1)
				{//				
					request_packet.packetdata.ack=1;
					request_packet.packetdata.ackseq=server_buffer[buffer_index].packetdata.sequence_num;
					last_sent=server_buffer[buffer_index].packetdata.sequence_num;
					acks[ack_pointer]=server_buffer[buffer_index].packetdata.sequence_num;
					if(ack_pointer<63)
					{
						ack_pointer++;
					}
					else
					{
						ack_pointer=0;
					}
					// prepare ack
					request_packet.checksum=crc((void *)&request_packet.packetdata,sizeof(struct packetdata),0);
					request_packet.msg_header=destaddr;
					sendto(requestsocket,(void *)&request_packet, sizeof(struct packet),0, (struct sockaddr *)&requestname,sizeof(requestname));
					server_window[lowest_seq_win_index]=-1;
					printf("\nAcknowledgement for Packet with SEQ num %d SENT\n",server_buffer[buffer_index].packetdata.sequence_num);
					
				}
				else if(server_buffer[buffer_index].packetdata.fin==1)
				{
					printf("\nReceived Fin Packet\n");
					
					sendto(localsock,(void *)&received_packet, sizeof(struct packet),0, (struct sockaddr *)&localname, sizeof(localname));
					
					request_packet.packetdata.finack=1;
					request_packet.packetdata.ack=0;
					request_packet.packetdata.ackseq=server_buffer[buffer_index].packetdata.sequence_num;
					request_packet.checksum=crc((void *)&request_packet.packetdata,sizeof(struct packetdata),0);
					request_packet.msg_header=destaddr;
					
					server_window[lowest_seq_win_index]=-1;
					
					
					sendto(requestsocket,(void *)&request_packet, sizeof(struct packet),0, (struct sockaddr *)&requestname,sizeof(requestname));
					strcmp(request_packet.packetdata.data,"FIN2");
					sendto(requestsocket,(void *)&request_packet, sizeof(struct packet),0, (struct sockaddr *)&requestname,sizeof(requestname));
					
					
					printf("FILE TRANSFER COMPLETE\n");
					
					
					
					close(trollsock);
					close(localsock);
					exit(0);
					
					
				}
			} 
			else              
			{ 
				printf("\n Waiting for lower packet \n");
				usleep(10000);
			} 
			
		}
		else if(match==0) // checksum does not match
		{
			printf(" \n Checksum does not match waiting for retransmission \n");
		}
		
		
		
		
	} 
	

} 

int find_low()
{
	int lowest=9999;
	for(i=window_back;i<=window_front;i++)
	if(server_window[i]<lowest && server_window[i]!=-1)
	lowest=server_window[i];

	return lowest;

}

int find_low_in_buffer(int low)
{
	for(i=tail;i<=63;i++)
	if(server_buffer[i].packetdata.sequence_num==low)
	return i;

}

int find_low_index(int lowest)
{
	int index;
	for(i=window_back;i<=window_front;i++)
	if(server_window[i]==lowest && server_window[i]!=-1)
	{
		index=i;	
	}

	return index;

}
void print_window()
{
	
	printf("\n WINDOW BACK %d->[%d] ",window_back,server_window[window_back]);
	printf("\n");
	for(i=window_back;i<=window_front;i++)
	{
		printf("%d/ ", server_window[i]);
	}
	printf("\n WINDOW Front %d -> %d",window_front,server_window[window_front]);
	printf("\n\n");
}



