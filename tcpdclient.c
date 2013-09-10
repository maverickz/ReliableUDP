#include "checksum.h"
#include "tcpd_def.h"
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

#define trollport 11000
#define ftpcport 9005
#define destinationport 12000
#define ackport 14000
#define timerportrecv 20000
#define timerportsend 21000
#define controlport 22000


int match; // flag for checksum
int head=0; // head of the buffer
int tail=0; // tail of the buffer
int i,k,j,l; // for loop declarations
int buffer_index; // index of the buffer
int window_back=0; 
int window_front=19;   // window related pointers
int window_pointer=0;
int flag;
int onlyflag;
int count;
int lastack;

// Functions

void print_window(); // function to print window
void print_buffer(); // function to print window
int window_empty();  // check if window is empty
void getcount();   // gets count
int buffer_not_empty();
int find_in_buffer(int sequence_num);
int window_only_fin(); // if window contains only fin

// buffer
struct packet buffer[64];

// window of size 20
int window[20]; 

main(int argc, char *argv[])
{
	int trollsock,localsock,requestsocket, controlsock;
	int total_bytes_sent,bytes_read;
	int bytes_recvd,bytes_written_to_file;
	int n,flag;
	int addr_len;
	int reply_seq_num;

	unsigned long received_checksum;	

	struct packet sending_packet;
	struct packet controlmessage; 
	struct packet request_packet; 
	struct sockaddr_in destaddr;
	struct sockaddr_in name;
	struct sockaddr_in controlname;
	struct sockaddr_in localname;
	struct sockaddr_in requestname;
	struct hostent *hp,*hp2,*hp3,*localhost, *control,*gethostbyname();

	fd_set read_fds; 

	
	for(i=0 ;i <64;i++)
	{
		buffer[i].packetdata.sequence_num=-1;
	}	

	for(i=0;i<20;i++)
	{
		window[i]=-1;	
	}
	
	struct timeval start_time, end_time, diff;
	int ack;
	double rem_time=0.0, start, end;

	
	int sock_tcpd_recv_timer,sock_tcpd_send_timer;
	int sequence_no;
	struct sockaddr_in tcpd_send_timer, tcpd_recv_timer;
	timerMess timer_mess;
	timerMess timer_reply;

	trollsock = socket(AF_INET, SOCK_DGRAM,0);
	if(trollsock < 0) {
		perror("opening datagram socket");
		exit(2);
	}

	/* construct name for connecting to server */
	name.sin_family = AF_INET;
	name.sin_port = htons(trollport);


	/* convert hostname to IP address and enter into name */
	hp = gethostbyname("127.0.0.1");
	if(hp == 0) {
		fprintf(stderr, "unknown host\n");
		exit(3);
	}
	bcopy((char *)hp->h_addr, (char *)&name.sin_addr, hp->h_length);



	controlsock = socket(AF_INET, SOCK_DGRAM,0);
	if(controlsock < 0) {
		perror("opening datagram socket");
		exit(2);
	}

	
	controlname.sin_family = AF_INET;
	controlname.sin_port = htons(controlport);


	
	control = gethostbyname("127.0.0.1");
	if(control == 0) {
		fprintf(stderr, "unknown host\n");
		exit(3);
	}
	bcopy((char *)control->h_addr, (char *)&controlname.sin_addr, control->h_length);


	localsock = socket(AF_INET, SOCK_DGRAM,0);
	if(localsock < 0) {
		perror("opening datagram socket");
		exit(2);
	}
	localname.sin_family = AF_INET;
	localname.sin_port = htons(ftpcport);
	localhost = gethostbyname("127.0.0.1");
	if(localhost == 0) {
		fprintf(stderr, "%s:unknown host\n", argv[1]);
		exit(3);
	}
	bcopy((char *)localhost->h_addr, (char *)&localname.sin_addr, localhost->h_length);
	
	if(bind(localsock, (struct sockaddr *)&localname, sizeof(localname)) < 0) {
		perror("Error binding localsock");
		exit(2);
	}
	int    localnamelen=sizeof(struct sockaddr_in);
	/* Find assigned port value and print it for client to use */
	// if(getsockname(localsock, (struct sockaddr *)&localname, &localnamelen) < 0){
		// perror("getting sock name");
		// exit(3);
	// }
	printf("TCPD CLIENT waiting on port # %d\n", ntohs(localname.sin_port));


	/* Fill-in my socket's address information to receive from timer*/
	tcpd_recv_timer.sin_family      = AF_INET;
	tcpd_recv_timer.sin_port        = ntohs(timerportrecv);
	tcpd_recv_timer.sin_addr.s_addr = INADDR_ANY;

	sock_tcpd_recv_timer = socket(AF_INET, SOCK_DGRAM,0);
	if(trollsock < 0) {
		perror("opening datagram socket");
		exit(2);
	}

	/* construct name for connecting to server */
	name.sin_family = AF_INET;
	name.sin_port = htons(trollport);


	/* convert hostname to IP address and enter into name */
	hp = gethostbyname("127.0.0.1");
	if(hp == 0) {
		fprintf(stderr, "unknown host\n");
		exit(3);
	}
	bcopy((char *)hp->h_addr, (char *)&name.sin_addr, hp->h_length);
	if( sock_tcpd_recv_timer <0 )
	{	printf("error opening datagram socket");
		exit(1);	
	}
	bind(sock_tcpd_recv_timer, (struct sockaddr *)&tcpd_recv_timer, sizeof(tcpd_recv_timer));


	/* Fill-in socket's address information to send to timer port */
	tcpd_send_timer.sin_family      = AF_INET;           	     /* Address family to use*/
	tcpd_send_timer.sin_port        = ntohs(timerportsend);    /* Port number of timer  */
	/* convert hostname to IP address and enter into name */
	hp3 = gethostbyname("127.0.0.1");
	if(hp3 == 0) {
		fprintf(stderr, "%s:unknown host\n", argv[1]);
		exit(3);
	}
	bcopy((char *)hp3->h_addr, (char *)&tcpd_send_timer.sin_addr, hp3->h_length);

	/* Create a socket on tcpdc for timer */
	sock_tcpd_send_timer = socket(AF_INET, SOCK_DGRAM, 0);
	bind(sock_tcpd_send_timer, (struct sockaddr *)&tcpd_send_timer, sizeof(tcpd_send_timer));

	hp2 = gethostbyname(argv[1]);
	if(hp2 == 0) {
		fprintf(stderr, "%s:unknown host\n", argv[1]);
		exit(3);
	}	
	
	bzero ((char *)&destaddr, sizeof destaddr);
	destaddr.sin_family = htons(AF_INET);
	bcopy(hp2->h_addr, (char*)&destaddr.sin_addr, hp2->h_length);
	destaddr.sin_port = htons(destinationport);

	requestsocket = socket(AF_INET, SOCK_DGRAM,0);
	if(trollsock < 0) {
		perror("opening datagram socket");
		exit(2);
	}
	
	/* construct requestname for connecting to troll */
	requestname.sin_family = AF_INET;
	requestname.sin_port = htons(ackport);
	requestname.sin_addr.s_addr= INADDR_ANY;

	/* convert hostname to IP address and enter into name */



	if(bind(requestsocket, (struct sockaddr *)&requestname, sizeof(requestname)) < 0) 
	{
		perror("getting socket name");
		exit(2);
	}
	int requestnamelen=sizeof(struct sockaddr_in);

	/* Find assigned port value and print it for client to use */
	if(getsockname(requestsocket, (struct sockaddr *)&requestname, &requestnamelen) < 0)
	{
		perror("getting sock name");
		exit(3);
	}
	bytes_recvd=1;    

	FD_ZERO(&read_fds);
	FD_SET(localsock, &read_fds);
	FD_SET(sock_tcpd_recv_timer, &read_fds);
	FD_SET(requestsocket, &read_fds);


	
	int firstPacket = 1;
	int resendpacket;
	

	while (1)
	{
		
		if (select(FD_SETSIZE, &read_fds, NULL, NULL, NULL) < 0 ) 
		{
			perror("Error in select");
			exit(2);
		}
		
		// if client sends data first
		if (FD_ISSET(localsock, &read_fds))
		{							
			// receive from local socket
			bytes_recvd=recvfrom(localsock, (void*)&buffer[head], sizeof(struct packet), 0, (struct sockaddr *)&localname, &localnamelen);
			
			// add sequence number to window array
			window[window_pointer]=buffer[head].packetdata.sequence_num;
			
			print_window();
			buffer[head].msg_header=destaddr;
			// calculate crc		
			buffer[head].checksum =crc((void *)&buffer[head].packetdata,sizeof(struct packetdata),0);
			// Find the packets index in the buffer		
			buffer_index=find_in_buffer(window[window_pointer]);
			
			sendto(trollsock,(void*)&buffer[buffer_index], sizeof(struct packet),0, (struct sockaddr *)&name, sizeof(name));
				
			gettimeofday(&start_time, NULL);
			
			// prepare timer message
			
			timer_mess.timeout = rto_get(rem_time,buffer[buffer_index].packetdata.sequence_num);
			timer_mess.seq_no = buffer[buffer_index].packetdata.sequence_num;
			timer_mess.port = timerportsend;

			printf ("\nSEQUENCE NUMBER OF THE PACKET SENT TO THE TIMER:%d\n",timer_mess.seq_no);
			n = sendto(sock_tcpd_send_timer, &timer_mess, sizeof(timer_mess), 0, (struct sockaddr *)&tcpd_send_timer, sizeof(tcpd_send_timer));
			
			window_pointer++;
			head=(head+1)%64;
			printf("Head : %d\n",head);
			if(window_pointer>=window_front)
			{
				printf("\nSLEEP SENT TO CLIENT\n");
				
				controlmessage.packetdata.stop=1;
				sendto(controlsock,(void*)&controlmessage, sizeof(struct packet),0, (struct sockaddr *)&controlname, sizeof(controlname));
			}
			else
			{
				controlmessage.packetdata.stop=0; //DONT SLEEP
				
				sendto(controlsock,(void*)&controlmessage, sizeof(struct packet),0, (struct sockaddr *)&controlname, sizeof(controlname));
				
			}			
		}		

		// if message is received from the timer

		if (FD_ISSET(sock_tcpd_recv_timer, &read_fds)) 
		{
			addr_len = sizeof(tcpd_recv_timer);
			printf("Waiting to receive from timer\n");
			n = recvfrom(sock_tcpd_recv_timer, &reply_seq_num, sizeof(reply_seq_num), 0, (struct sockaddr*)&tcpd_recv_timer, &addr_len);
			if (n>0) 
			{
				printf("\nTIMER EXPIRED FOR PACKET %d\n",reply_seq_num);
			}

			// obtain the sequence number of the packet who timed out and find it in the window 
			// after it is found send it again from the buffer
			for(i=window_back;i<=window_front;i++) // search entire window
			{
				if(window[i]==reply_seq_num) // found packet in the window
				{
					
					for (j=0;j<=63;j++) // start finding the packet in the buffer
					{
						if((buffer[j].packetdata.sequence_num==reply_seq_num) && (buffer[j].packetdata.sequence_num!=-1))
						{
							printf("\n PACKET TO RESEND FROM THE BUFFER %d\n",buffer[j].packetdata.sequence_num);
							resendpacket=j;	 //resend packet consists of the index of the packet to be resent
						}
					}				
					
				}
				
			}
			
			//Re-Send message to troll 
			total_bytes_sent=sendto(trollsock,(void*)&buffer[resendpacket], sizeof(struct packet),0, (struct sockaddr *)&name, sizeof(name));
			printf("\nPacket %d RESENT to troll \n",buffer[resendpacket].packetdata.sequence_num);				
			// start time	
			gettimeofday(&start_time, NULL);
			// prepare timer message
			timer_mess.timeout = rto_get(rem_time,buffer[resendpacket].packetdata.sequence_num);
			timer_mess.seq_no = buffer[resendpacket].packetdata.sequence_num;
			timer_mess.port = timerportsend;

			printf ("Seq No of timer:%d\n",timer_mess.seq_no);
			// Send msg to timer
			printf("Waiting to re-send to timer\n");
			n = sendto(sock_tcpd_send_timer, &timer_mess, sizeof(timer_mess), 0, (struct sockaddr *)&tcpd_send_timer, sizeof(tcpd_send_timer));
		} 
		

		// receive Acknowledges and Retransmission requests here	
		
		if (FD_ISSET(requestsocket, &read_fds)) 
		{
			
			// ONCE A PACKET IS ACKNOWLEDGED 
			// 1. IT HAS TO BE DELETED FROM THE TIMER LIST
			// 2. IT HAS TO BE REMOVED FROM THE BUFFER
			// 3. THE WINDOW SHOULD MOVE OVER IT
			gettimeofday(&end_time,NULL);
			/* Use the time to calculate measured RTT*/
			rem_time = timeval_diff(NULL, &end_time, &start_time);
			
			recvfrom(requestsocket, (void*)&request_packet, sizeof(request_packet), 0, (struct sockaddr *)&requestname, &requestnamelen);

			received_checksum=crc((void *)&request_packet.packetdata,sizeof(struct packetdata),0);
			
			if(request_packet.checksum==received_checksum)
			match=1;
			else
			match=0;
			
			if(match==1)
			{
				if(request_packet.packetdata.ack==1)				
				{
					
					// find the sequence number of the packet inside the window and then write over it
					// make window shift
					// DELETE THE NODE FROM THE TIME LIST
					timer_mess.timeout = 0;  /* 0 => delete */
					timer_mess.seq_no = request_packet.packetdata.ackseq;
					timer_mess.port = timerportsend;		
					// printf("\nPACKET %d ACKNOWLEDGED\n",request_packet.packetdata.ackseq);	
					
					// SEND MESSAGE TO TIMER
					n = sendto(sock_tcpd_send_timer, &timer_mess, sizeof(timer_mess), 0, (struct sockaddr *)&tcpd_send_timer, sizeof(tcpd_send_timer));
					if(n>0)
					{
						printf("\nPacket %d Acknowledged- Deleted from the delta list \n",request_packet.packetdata.ackseq);
					}

					for(k=window_back;k<=window_front;k++)
					{
						if(window[k]==request_packet.packetdata.ackseq)
						{
							window[k]=-1;
						}
					}
					
					if(window_empty()) 
					{	
						controlmessage.packetdata.stop=0; //DONT SLEEP
						window_pointer=0;
						sendto(controlsock,(void*)&controlmessage, sizeof(struct packet),0, (struct sockaddr *)&controlname, sizeof(controlname));
					}
					
				}

				if(request_packet.packetdata.finack==1 ) //logic for fin has to come here;
				{	 
					printf("\nFIN received\n");
					recvfrom(requestsocket, (void*)&request_packet, sizeof(request_packet), 0, (struct sockaddr *)&requestname, &requestnamelen);
					if(strcmp(request_packet.packetdata.data,"FIN2"))
					{						
						printf("\nSLEEP SENT TO CLIENT\n");
						
						controlmessage.packetdata.stop=1;
						sendto(controlsock,(void*)&controlmessage, sizeof(struct packet),0, (struct sockaddr *)&controlname, sizeof(controlname));
						timer_mess.timeout = 0;  /* 0 => delete */
						timer_mess.seq_no = request_packet.packetdata.ackseq;
						timer_mess.port = timerportsend;
						
						n = sendto(sock_tcpd_send_timer, &timer_mess, sizeof(timer_mess), 0, (struct sockaddr *)&tcpd_send_timer, sizeof(tcpd_send_timer));	
						
						printf("FILE TRANSFER COMPLETE\n");
						
						
						close(trollsock);
						close(localsock);
						exit(0);
					}
					
				}
			}
			
		}
		
		
		FD_ZERO(&read_fds);
		FD_SET(localsock, &read_fds);
		FD_SET(sock_tcpd_recv_timer, &read_fds);
		FD_SET(requestsocket, &read_fds);
		
	}


}// end main

void print_buffer()
{
	
	printf(" TAIL %d->[%d] ",tail,buffer[tail].packetdata.sequence_num);
	printf("\n");
	for(i=0;i<=63;i++)
	{
		printf(" [%d] ",buffer[i].packetdata.sequence_num);
	}
	printf("\n");
	printf(" HEAD %d-> [%d] ",head,buffer[head].packetdata.sequence_num);		
	printf("\n");
	
	
}

void print_window()
{
	
	printf(" WINDOW BACK %d->[%d] ",window_back,window[window_back]);
	printf("\n");
	for(i=window_back;i<=window_front;i++)
	{
		printf("%d/ ", window[i]);
	}
	printf("\n WINDOW Front %d -> %d",window_front,window[window_front]);
	
}



int find_in_buffer(int seq)
{
	for(i=0;i<=63;i++)	
	if(buffer[i].packetdata.sequence_num==seq)			
	{	
		break;				
	}
	return i;
}

void getcount()
{
	count=0;
	for(i=window_back;i<=window_front;i++)
	{
		if(window[i]!=-1)			
		count++;
	}

}

int window_empty()
{
	flag=0;
	for(i=window_back;i<=window_front;i++)
	{
		if(window[i]!=-1)			
		flag=1;
	}

	if(flag==1)
	{
		return 0;
	}
	else
	{
		return 1;
	}

}

int window_only_fin()
{
	for(i=window_back;i<=window_front;i++)
	{
		if(window[i]!=-1 && buffer[window[i]].packetdata.fin!=1)
		onlyflag=1;
		
	}	
	if(onlyflag)
	return 0;
	else 
	return 1;	
	
}
