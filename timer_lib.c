
#include <stdio.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* define structure types */
typedef struct _peer {
	int id;
	int port;
	double time_out;
	struct _peer *prev;
	struct _peer *next;
} Peer;

typedef struct _peer_list {
	int numPeers;
	struct _peer *head;
	struct _peer *tail;
} PeerList;


void print_list(PeerList* list) {
	Peer* temp;
	temp = list->head;

	/* If there are elements in list */
	if (list -> head != NULL) {
		printf ("\n******************* START LIST ***************\n");
		while(temp != NULL) {
			printf("Sequence Number:%-5d",temp->id);
			printf("Time Out:%f\n\n",temp->time_out);
			temp = temp->next;
		}
		printf ("***************** END LIST *******************\n");
	}
}/* print_list end */


Peer* newPeer(int id, int port, double timeout)
{
	Peer *fp = NULL;
	/* Allocate memory for node */
	if ((fp = (Peer *) malloc(sizeof(Peer))) != NULL) {
		fp->id = id;
		fp->port = port;
		fp->time_out = timeout;
		fp->prev = NULL;
		fp->next = NULL;
		if(fp->time_out !=0)
		printf("\nTimer Created for packet no:%d with timeout :%f\n", fp->id, fp->time_out);
	} else {
		fprintf(stderr, "Error: Memory allocation failed!\n");
	}
	return fp;
}


void delPeer(Peer *fp)
{
	/* Delete if not null */
	if (fp != NULL) {
		fp->prev = NULL;
		fp->next = NULL;
		free(fp);
	}
}

PeerList* newPeerList()
{
	PeerList *list = NULL;

	list = (PeerList *) malloc(sizeof(PeerList));
	if (list != NULL) {
		list->numPeers = 0;
		list->head = NULL;
		list->tail = NULL;
		printf("Timer list created\n");
	} else {
		fprintf(stderr, "Error: Memory allocation failed!\n");
	}
	return list;
}/*newPeerList end*/


void delPeerList(PeerList *list)
{
	Peer *fp;
	/*delete list if not empty*/
	if (list != NULL) {
		for (fp = list->head; fp != NULL; fp = list->head) {
			list->head = list->head->next;
			delPeer(fp);
		}
		free(list);
	}
}/*delPeerList end*/


int addPeerToList(PeerList *list, Peer *fp)
{
	Peer *temp, *prev1;
	int total_to=0;
	/* First node */ 
	if (list->head == NULL) {
		assert(list->tail == NULL);
		list->head = list->tail = fp;
	} 
	/* Insert at the start of the list if timeout value is less than the head node */
	else if (fp -> time_out <= list->head->time_out) {
		list->head->time_out -= fp -> time_out;
		fp -> next = list->head;
		list -> head -> prev = fp; 
		list -> head = fp;
		fp -> prev = NULL;

	}  
	/* Insert in the list at its appropriate position*/
	else {
		temp = list->head;  
		total_to=temp->time_out;
		while(fp->time_out>total_to && temp->next != NULL) {
			prev1=temp; 
			temp=temp->next;
			total_to += temp->time_out;
		}
		if(temp->next!= NULL){
			fp -> time_out -= total_to;
			//  fp -> time_out += temp -> time_out;
			temp -> time_out -= fp -> time_out; 
			fp->next=temp; 
			prev1->next=fp;
			fp->prev=prev1;
			temp->prev=fp; 
		}    
		else {
			if (fp->time_out < total_to) {
				fp ->time_out -= total_to;
				//     fp -> time_out += temp->time_out;
				temp -> time_out -= fp->time_out;
				prev1 -> next = fp;
				fp -> next = temp;
				temp -> prev = fp;
				fp -> prev = prev1;
			} 
			/*Insertion at the end of the list*/
			else {
				fp -> time_out -= total_to;
				list->tail->next = fp;
				fp->prev = list->tail;
				fp -> next = NULL;
				list->tail = fp;
			}
		}
	} 
	list->numPeers++;
	printf("Timer node added to list\n");
	return 1;
}


int peerExists(PeerList *list, int id)
{
	Peer *p;
	p = list->head;

	while (p != NULL) {
		/* Match sequence number of nodes*/
		if (p->id == id) {
			printf("\nPeer no :%d found in delta list",id);
			return 1;
		}
		p = p->next;
	}
	return 0;
}/*peerExist end*/


int delPeerFromList(PeerList *list, Peer *fp, int ack)
{
	Peer *forwardPeer, *prev;
	int found = 0;
	/*Check if list is empty*/
	if (list->numPeers == 0) {
		return 0;
	}
	prev = NULL;
	forwardPeer = list->head;
	while (forwardPeer != NULL) {
		/* found it */
		if (forwardPeer->id == fp->id) {
			found = 1;
			/* delete head */
			if (prev == NULL) 
			{
				/*if only one node in the list*/
				list->head = forwardPeer->next;
				if (list->head == NULL) {
					list->tail = NULL;
					break;
				}
				else {
					if (ack==1)
					/*Update the list of head if the head node is deleted*/
					forwardPeer->next->time_out += forwardPeer -> time_out;
					forwardPeer->next->prev = NULL;
				}
			}
			/* delete tail */
			else if (forwardPeer->next == NULL) {
				list->tail = prev;
				prev->next = NULL;
			}
			/* delete a path in the middle */
			else {
				if (ack==1) 
				forwardPeer->next->time_out += forwardPeer -> time_out;
				prev->next = forwardPeer->next;
				forwardPeer->next->prev = prev;
			}

			delPeer(forwardPeer);
			list->numPeers--;
			printf ("\n Timer node deleted from delta timer\n");
			break;
		}
		prev = forwardPeer;
		forwardPeer = forwardPeer->next;
	}
	return found;
}/*delPeerFromList end*/



void UpdateList(PeerList *list) {
	Peer *forwardPeer, *temp;
	int n;
	struct sockaddr_in tcpd_addr;/* socket descriptot */
	struct hostent *hp, *gethostbyname();
	int sockf;	

	/*Create a socket for the timer process*/
	sockf = socket(AF_INET, SOCK_DGRAM, 0); 
	// if( sockf <0){
		// printf("\n error creating socket");
	// }

	//printf("\n socket created");
	/*Fill local information to send data to client daemon*/
	tcpd_addr.sin_family      = AF_INET;           	     /* Address family to use*/
	tcpd_addr.sin_port        = htons(20000);                 /* Port number of timer  */
	/* convert hostname to IP address and enter into name */
	hp = gethostbyname("127.0.0.1");
	if(hp == 0) {
		//fprintf(stderr, "%s:unknown host\n", );
		exit(3);
	}
	bcopy((char *)hp->h_addr, (char *)&tcpd_addr.sin_addr, hp->h_length);

	/*list empty*/
	if (list->head == NULL) {
		// printf("No Peer\n");
	} 
	else {
		/*update the time in the list*/
		// printf ("Before Update\n"); 
		forwardPeer = list -> head;
		forwardPeer -> time_out -= 10000;

		/*Check for nodes whose timeout value has expired*/
		while (forwardPeer -> time_out <=0 && forwardPeer != NULL) {
			printf ("\n Node timedout:%d",forwardPeer->id);
			
			/* Inform client the packet id's for which timeout has expired*/
			 
			n =  sendto(sockf, &(forwardPeer->id), sizeof(forwardPeer->id), 0, (struct sockaddr*)&tcpd_addr, sizeof(tcpd_addr));
			//usleep(20000);
			if(n<0) {
				printf("Seq num: %d\n",forwardPeer->id);
				perror("Error in sendto\n");
			}
			else 
				printf("\nMessage sent to tcpdc to retransmit packet no %d",forwardPeer->id);
			
			/*delete the nodes for which timeout has occurred*/
			temp = forwardPeer;
			forwardPeer = forwardPeer -> next;
			if (temp -> next != NULL) {
				//    forwardPeer -> time_out += temp -> time_out;
			}
			delPeerFromList(list, temp,0);
			if (forwardPeer==NULL)
				break;
			
		}//while
		

		if (forwardPeer != temp) 
		list -> head = forwardPeer;
		close(sockf);
	}//else
}/*UpdateList end*/
