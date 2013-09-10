/* TCPD library*/

/*Include all the requird header files*/
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netinet/in.h>

/* Timer variables */
#define timerportrecv 20000

/* Structure defining the timer message */
typedef struct _timerMess {
  int seq_no;
  int port;
  int timeout;
} timerMess;


/***************************************************************************
* Function: timer()
* Arguments:    seq_no -> Sequence no of the packet
*               sockfd -> Socket id
*               timeout -> time out period for the packet
* Returns:      void
* Description:  This function forms the packet and sends it to the timer process to add or delete the node
****************************************************************************/

void timer (int seq_no, int sockfd, unsigned int timeout) {

  timerMess message;
  struct sockaddr_in timer_addr;
  int n;

  timer_addr.sin_family = AF_INET;
  timer_addr.sin_port = htonl(timerportrecv);
  timer_addr.sin_addr.s_addr =0;

  message.seq_no = seq_no;
  message.port = sockfd;
  message.timeout = timeout;

  n = sendto(sockfd, &message, sizeof(message), 0, (struct sockaddr *)&timer_addr, sizeof(timer_addr));

  if (n>0) {
    if (timeout == 0 ) {
      printf("Send Cancel\n");
    } else {
      printf("Send add to timer \n");
    }
  }
}

/***************** End of Timer *******************************************/




