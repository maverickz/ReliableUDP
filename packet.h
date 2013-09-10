#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

struct packetdata
{
int sequence_num;
int ack;
int ackseq;
int bytes_in_packet;
int fin;
int finack;
int stop;
char data[900];
};

struct packet
{
struct sockaddr_in msg_header;
struct packetdata packetdata;
unsigned long checksum;
};
//

//
int SEND(int localsock, struct packet *sending_packet, int len, int flags) 
{

	  struct sockaddr_in name;
	  int  total_bytes_written;


  /* Create the tcpdclient address to send information */
         name.sin_family = AF_INET;
    	 name.sin_port = htons(9005);
 	 name.sin_addr.s_addr = INADDR_ANY;
  /* Send data to tcpd client */
	  total_bytes_written=sendto(localsock,(void *)sending_packet, sizeof(struct packet),flags, (struct sockaddr *)&name, sizeof(name));


  /* Check for error */
	  if ( total_bytes_written<0) 	
	{
	    printf("Send Error\n");
	    exit(1);
	} 
	else 
	{
	    return  total_bytes_written;
	}
}
#if 0
int RECV(int localsock, struct packet *received_packet, int len, int flags) 
{

	 struct sockaddr_in localname2;
	 int total_bytes_returned; 
	 localname2.sin_family = AF_INET;
	 localname2.sin_port = htons(13000);
	 localname2.sin_addr.s_addr = INADDR_ANY;
	 int localnamelen=sizeof(struct sockaddr_in);


  /* Receive data from the tcp daemon */
	 total_bytes_returned=recvfrom(localsock,(void *)received_packet, sizeof(struct packet), 0, (struct sockaddr *)&localname2, &localnamelen);
	 
  /* Error check */
	if (total_bytes_returned<0) 
	{
	    printf("Receive failed\n");
	    exit(1);
  	} 
	else 
	{
	return total_bytes_returned;
  	}

}/* RECV end */
#endif

int RECV_C(int localsock, struct packet *received_packet, int len, int flags) 
	{

	    struct sockaddr_in localname2;
            int total_bytes_returned; 
	    localname2.sin_family = AF_INET;
	    localname2.sin_port = htons(22000);
	    localname2.sin_addr.s_addr = INADDR_ANY;
	    int localnamelen=sizeof(struct sockaddr_in);

	  /* Receive data from the tcp daemon */
	 total_bytes_returned=recvfrom(localsock,(void *)received_packet, sizeof(struct packet), 0, (struct sockaddr *)&localname2, &localnamelen);
	
	  /* Error check */
            if (total_bytes_returned<0) 
		{
		    printf("Receive failed\n");
		    //exit(1);
		}
	 else 
		{
		return total_bytes_returned;
		}

}/* RECV_C end */


