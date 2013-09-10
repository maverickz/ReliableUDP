struct tcpheader {
 unsigned short int sourcePort;
 unsigned short int destinationPort;
 unsigned int       sequenceNumber;
 unsigned int       ackNumber;
 unsigned char      tcph_offset:4, reserved:4;
 unsigned int
       tcph_fin:1,       /*Finish flag "fin"*/
       tcph_syn:1,       /*Synchronize sequence numbers to start a connection*/
       tcph_rst:1,       /*Reset flag */
       tcph_psh:1,       /*Push, sends data to the application*/
       tcph_ack:1,       /*acknowledge*/
       tcph_urg:1,       /*urgent pointer*/
       tcph_res2:2;
 unsigned short int window;
 unsigned short int checkSum;
 unsigned short int urgPtr;
};
