/* TCPD library*/

/*Include all the requird header files*/
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netinet/in.h>

/* Variables for rto computation */
double rto = 6000;

/* CRC variables*/
#define                 P_CCITT     0x1021
static int              crc_tabccitt_init=0;
static unsigned short   crc_tabccitt[256];
static void             init_crcccitt_tab( void );

/* Timer variables */
# define PORT_NUM_TIMER_SEND 9050

/* Structure defining the timer message */
typedef struct _timerMess {
  int seq_no;
  int port;
  double timeout;
} timerMess;




double rto_get(double m, int pktcntr) {
  float a=0, d=3000, err;
  const float g = 0.125;
  const float h = 0.25;
//printf("Lib cntr:%d\n",pktcntr);

  /* First packet */
  if ( pktcntr == 0) {
    printf("\nRTO:%f\n",rto);
        return rto;
  } else if(pktcntr==1)
  {
     a = 50;
    if( m ==0)	
    	rto = 6000;
    else 
	rto = m;	
    //printf("\nRTO:%f\n",rto);
     return rto;
  }
  else
  {
        err = m-a;
        a = a + g * err;
        d = d+h*(abs(err)-d);
   }
  rto =a+4*d;

  return rto;
}
/* rto_get end */


double timeval_diff(struct timeval *difference,
             struct timeval *end_time,
             struct timeval *start_time
            )
{
  struct timeval temp_diff;

printf( "\nend is : %ld.%.6ld", end_time->tv_sec, end_time->tv_usec);
printf( "\nstart is : %ld.%.6ld", start_time->tv_sec, start_time->tv_usec);
double time = (end_time->tv_sec * 1000000 + end_time->tv_usec)
		  - (start_time->tv_sec * 1000000 + start_time->tv_usec);
printf("\nRTT is %f",time);

return time;


} 



