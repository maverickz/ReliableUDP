/* Timer process */

/* Include all required files */
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <signal.h>
#include "timer_lib.c"
#define port1 21000

/***************** TIMER PACKET ****************/

typedef struct _packet {
	int seq_no;
	int port;
	double timeout;
} packet;
/***************** TIMER PACKET ****************/

int main(void) {
	/* Declaration of variables */ 
	int n, addr_len;
	packet pkt;

	/* timer variables */
	unsigned int start, end, rem_time=100;
	struct timeval tv, start_time, end_time;

	fd_set rfds;/* select variable */
	int ret;

	/*socket variables*/
	unsigned int sock, sock1;
	struct sockaddr_in tcpd_addr, timer_addr; 

	/* Variables for buffer */
	PeerList *list;
	Peer *node;

	/* Fill-in socket's address information for timer port */
	timer_addr.sin_family = AF_INET;
	timer_addr.sin_port = ntohs(port1);
	timer_addr.sin_addr.s_addr = INADDR_ANY;
	/* Create a new list */
	list = newPeerList();

	/*Create a socket for the timer process*/
	sock = socket(AF_INET, SOCK_DGRAM, 0); 
	if(sock <0){
		printf("\n error creating socket");
		exit(2);
	}else 
	printf("\n socket created\n");

	if(bind(sock,(struct sockaddr *)&timer_addr, sizeof(timer_addr)) < 0)
	{
		printf("\nbind error occured\n");
		exit(2);
	}  


	while (1) {
		if(rem_time == 100) {
			tv.tv_sec = 0;
			tv.tv_usec = 100000;
		} else { 
			tv.tv_sec = 0;
			tv.tv_usec = (rem_time*1000);
		}

		// Initialize read sockets      
		FD_ZERO(&rfds);
		FD_SET(sock, &rfds);
		gettimeofday(&start_time, NULL);
		ret = select(FD_SETSIZE, &rfds, NULL, NULL,&tv);

		// Receive data from the client                      
		if (ret == -1) {
			printf("\nnothing to read");
		} 
		else if (ret) {
			usleep(10000);
			printf("\nWaiting for data from tcpd client");
			//receive timer information from tcpd client
			n = recvfrom(sock, &pkt, sizeof(pkt), 0, (struct sockaddr*)&timer_addr, &addr_len);
			if (n<0) {
				printf("No data received\n");
			} 
			else 
			{

				// Create a new node 
				node = newPeer(pkt.seq_no, pkt.port, pkt.timeout);
				
				// Add node to the list 
				if ( pkt.timeout != 0 ) {
					if (addPeerToList(list, node)< 1) {
						printf("Cannot add node to list\n");
						exit(1);
					} 
					//print the list created so far
					print_list(list);

				} else {

					//* If timeout is zero, delete from the list *
					if (peerExists(list, pkt.seq_no)==1) {
						if (delPeerFromList(list, node,1)>0) {
							print_list(list);
							printf("\nTimer deleted from delta list");
						} else {
							printf("Cannot delete timer\n");
						}
					} else {
						printf("Timer dosent exist in list\n");
					}
				}
				gettimeofday(&end_time, NULL);

			}
		}
		else {
			// if no message is received from the client and timeout has occured
			UpdateList(list);
			//usleep(1000);
			print_list(list);
			rem_time = 100;
			//printf("\nDelta timer running\n");
		}
	} 

	return 0;
}
