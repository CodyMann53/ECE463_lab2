#include "router.h" 
#include <pthread.h>
#include <time.h>

#define ROUTER_ID_ARGV_POSITION 1
#define DEST_HOSTNAME_ARGV_POSITION 2
#define DEST_PORT_ARGV_POSITION 3
#define ROUTER_DEST_ID_ARGV_POSITION 4
int main (int argc, char ** argv){
	// Convert a port number command line args to integers
	int router_id = atoi(argv[ROUTER_ID_ARGV_POSITION]);
	int dest_port = atoi(argv[DEST_PORT_ARGV_POSITION]);
	struct sockaddr_in dest_addr; 
	struct hostent *hp;	
	struct pkt_RT_UPDATE pkt; 
	bzero(&pkt, sizeof(pkt)); 
	int clientfd; 	

	// Create a socket for udp client 
  	if ((clientfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
    		perror("Error in trying to create UDP socket.\r\n"); 
    		return -1;
  	}

	// Get the network emulator's address 
	if ((hp = gethostbyname(argv[DEST_HOSTNAME_ARGV_POSITION])) == NULL ){ 
		perror("Error when getting hostname"); 
		pthread_exit(0); 
	}

	// Fill up the network emulate address structure
	memset(&dest_addr, 0, sizeof(dest_addr)); 
	dest_addr.sin_family = AF_INET; 
	dest_addr.sin_port = htons((unsigned short) dest_port);
	bcopy((char * ) hp->h_addr, (char *) &dest_addr.sin_addr.s_addr, hp->h_length); 	

			
	// Create an update packet and then send it to all neighbors 
	bzero(&pkt, sizeof(pkt)); 
	ConvertTabletoPkt(&pkt, router_id);

	// set the destination, and then send message to network emulator 				
	pkt.dest_id = atoi(argv[ROUTER_DEST_ID_ARGV_POSITION]); 	
	hton_pkt_RT_UPDATE(&pkt);
	sendto(clientfd, &pkt, sizeof(struct pkt_RT_UPDATE), MSG_CONFIRM, (const struct sockaddr * ) &dest_addr, sizeof(dest_addr)); 

	close(clientfd);  
	return 0; 
}
