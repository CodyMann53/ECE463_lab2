#include "router.h" 
#include <pthread.h>
#include <time.h>

#define ROUTER_ID_ARGV_POSITION 1
#define NE_HOSTNAME_ARGV_POSITION 2
#define NE_UDP_PORT_ARGV_POSITION 3
#define ROUTER_UDP_PORT_ARGV_POSITION 4


// Global variables
pthread_t udp_server_tid; 
pthread_t time_keeper_tid; 
pthread_mutex_t lock; 

// Thread shared memory, need to use locking when accessing these variables 
int update_interval_flag = 0; 


struct args{
	int udp_port;
	int ne_udp_port; 
	int router_id;
	int sockfd;  
	char ne_hostname[PACKETSIZE]; 
};  

// sets up a udp server listening on passed in port and all hostnames
int open_fd_udp(int port);


void * udp_server_thread(void * param); 


void send_update_packets(struct pkt_RT_UPDATE *UpdatePacketToSend, struct sockaddr_in * ne_addr, int fd); 


void * time_keeper_thread(void * param); 

 
void init_router(int router_id, int ne_udp_port, int router_udp_port, char * ne_hostname); 


void create_pkt_INIT_RESPONSE(char * buffer, int  n, struct pkt_INIT_RESPONSE * pkt); 


unsigned int extract_unsigned_int(char * buffer, int buffer_position); 


void print_pkt_INIT_RESPONSE(struct pkt_INIT_RESPONSE * pkt); 


int main (int argc, char ** argv){
	// Convert a port number command line args to integers
	int router_id = atoi(argv[ROUTER_ID_ARGV_POSITION]);
	int ne_udp_port = atoi(argv[NE_UDP_PORT_ARGV_POSITION]);
	int router_udp_port = atoi(argv[ROUTER_UDP_PORT_ARGV_POSITION]); 
	struct sockaddr_in ne_addr; 
	struct hostent *hp;	
	struct pkt_RT_UPDATE pkt; 
	bzero(&pkt, sizeof(pkt)); 
	
	// Initialize the router 
/*	init_router(router_id, ne_udp_port, router_udp_port, argv[NE_HOSTNAME_ARGV_POSITION]); 

	// if debugging, then log tables after initialization
#if (DEBUG == 1)
	char file_name[100]; 
	bzero(file_name, 100); 
	sprintf(file_name, "init_router_%d.log", router_id); 	
	FILE * fp = fopen(file_name, "w"); 
	PrintRoutes(fp, router_id);
	fclose(fp);  
#endif
*/	
/*	// Set up arguments for threads 
	struct args arguments; 
	bzero(&arguments, sizeof(struct args)); 
	arguments.udp_port = router_udp_port;
	arguments.ne_udp_port = ne_udp_port;
	arguments.router_id = router_id;  
	strcpy(arguments.ne_hostname, argv[NE_HOSTNAME_ARGV_POSITION]);  	
	
	// Create a sock file descriptor for udp communication between router and network emulator
	arguments.sockfd = open_fd_udp(arguments.udp_port); 
	
	// Create the udp server and time keeper thread
	pthread_attr_t attr; 
	pthread_attr_init(&attr); 
	pthread_create(&udp_server_tid, &attr, udp_server_thread,  &arguments);  
	pthread_create(&time_keeper_tid, &attr, time_keeper_thread, NULL); 
		
	// Get the network emulator's address 
	if ((hp = gethostbyname(arguments.ne_hostname)) == NULL ){
		close(arguments.sockfd); 
		perror("Error when getting hostname"); 
		pthread_exit(0); 
	}

	// Fill up the network emulate address structure
	memset(&ne_addr, 0, sizeof(ne_addr)); 
	ne_addr.sin_family = AF_INET; 
	ne_addr.sin_port = htons((unsigned short)arguments.ne_udp_port);
	bcopy((char * ) hp->h_addr, (char *) &ne_addr.sin_addr.s_addr, hp->h_length); 	
	
	// Event while loop that performs the path vector alrogithm 
	while (1){
		// Check on timer
		pthread_mutex_lock(&lock); 
		if (update_interval_flag == 1){
			// Create an update packet and then send it to all neighbors 
			bzero(&pkt, sizeof(pkt)); 
			ConvertTabletoPkt(&pkt, arguments.router_id);
			

			send_update_packets(&pkt, &ne_addr, arguments.sockfd);  			
	
			// Reset the update interval timer flag
			update_interval_flag = 0; 		

		}	
		pthread_mutex_unlock(&lock); 
	}

	// Join threads and close socket file descriptor 
	pthread_join(udp_server_tid, NULL); 
	pthread_join(time_keeper_tid, NULL);  
	close(arguments.sockfd);  */
	return 0; 
}


void * udp_server_thread(void * param){
	struct sockaddr_in cli_addr; 	
	int n, len; 
	char buffer[PACKETSIZE];
	
	// Retrieve passed in arguments
	struct args * arguments  = (struct args *) param;
	
	// Zero out buffers and structures 
	memset(&cli_addr, 0, sizeof(cli_addr)); 
	bzero(buffer, PACKETSIZE); 
	
	// Start receiving incoming router update messages		
	while (1){
		// Wait for a packet 
		memset(buffer, 0, PACKETSIZE); 
		len = sizeof(cli_addr); 
		n = recvfrom(arguments->sockfd, (char *) buffer, (size_t) PACKETSIZE, MSG_WAITALL, (struct sockaddr *) &cli_addr, (socklen_t *) &len);
		printf("Received %d bytes from neighbor\r\n", n);  
	}
	
	// Close socket and return from thread 
	pthread_exit(0);
}


void send_update_packets(struct pkt_RT_UPDATE *UpdatePacketToSend, struct sockaddr_in * ne_addr, int fd){



	sendto(fd, "Hello from router\r\n", sizeof("Hello from router\r\n"), MSG_CONFIRM, (const struct sockaddr *) ne_addr, sizeof(*ne_addr)); 
	
	return; 
}


void * time_keeper_thread(void * param){
	// Get base time for needed timers
	time_t base_time_update_interval = time(NULL);
	
	// Event loop to check on timers
	while(1){
		// check on update interval timer
		if ((time(NULL) - base_time_update_interval) > UPDATE_INTERVAL){
			// Provide signal that update interval timer has completed			  		
			pthread_mutex_lock(&lock); 
			update_interval_flag = 1; 		
			pthread_mutex_unlock(&lock); 

			// reset the time base to restart the update interval timer
			base_time_update_interval = time(NULL); 
		}
	}
	pthread_exit(0);
}


unsigned int extract_unsigned_int(char * buffer, int buffer_position){
	int x; 
	unsigned int tmp = 0; 
	for (x = 0; x < sizeof(unsigned int); x++){
		tmp = tmp | (buffer[buffer_position + x] << 8*x);  
	}
	return tmp; 
}


void print_pkt_INIT_RESPONSE(struct pkt_INIT_RESPONSE * pkt){
	if (pkt == NULL){
		printf("Null pointer when trying to print pkt_INIT_RESPONSE\r\n");
		return;  
	}
	
	int z; 
	printf("Number of neighbors: %d\r\n", pkt->no_nbr); 
	for (z = 0; z < pkt->no_nbr; z++){
		printf("Neighbor id: %d\r\n", pkt->nbrcost[z].nbr);  
		printf("Cost to neighbor: %d\r\n", pkt->nbrcost[z].cost);  
	}
	return; 	
}


void create_pkt_INIT_RESPONSE(char * buffer, int  n, struct pkt_INIT_RESPONSE * pkt){
	// extract number of neighbors from buffer
	int i;
	int pos_cnt = 0; 
	int neighbor_counter = 0; 
	unsigned int tmp  = 0;   
	
	// check for null buffer and packet pointers
	if (buffer == NULL){
		printf("Buffer was null when calling create_pkt_INIT_RESPONSE\r\n"); 
		return; 
	}
	if (pkt == NULL){
		printf("pkt_INIT_RESPONSE pointer was null when calling create_pkt_INIT_RESPONSE\r\n"); 
		return; 
	}

	// Loop through all of the buffer bytes 
	for (i = 0; i < sizeof(struct pkt_INIT_RESPONSE); i += sizeof(unsigned int)){
		// if on first few bytes for number of neighbors, then extract number of neighbors
		if ( i == 0){
			tmp = extract_unsigned_int(buffer, i); 
			pkt->no_nbr = htonl(tmp); 	
		}
		// else extract the neighbor id and cost 
		else{
			// extracting neighbor id
			if (pos_cnt == 0){
				tmp = extract_unsigned_int(buffer, i);
				pkt->nbrcost[neighbor_counter].nbr = htonl(tmp);  
			}
			// extracting cost to neighbor
			else{
				tmp = extract_unsigned_int(buffer, i); 
				pkt->nbrcost[neighbor_counter++].cost = htonl(tmp);  
			}
			pos_cnt = (pos_cnt + 1) % 2; 
		}
	}

	return; 
} 	


void init_router(int router_id, int ne_udp_port, int router_udp_port, char * ne_hostname){
	int sockfd_client; 
	char buffer[PACKETSIZE];
	int n, len; 
	struct sockaddr_in servaddr, cliaddr; 
  	struct hostent *hp;
	struct pkt_INIT_RESPONSE pkt_init_response;
	bzero(&pkt_init_response, sizeof(struct pkt_INIT_RESPONSE));  

	// creating socket file descriptor
	if ( (sockfd_client = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("socket creation failed"); 
		exit(EXIT_FAILURE); 
	}
	
	memset(&servaddr, 0, sizeof(servaddr)); 
	memset(&cliaddr, 0, sizeof(cliaddr)); 
	
	// filling server information
	if ((hp = gethostbyname(ne_hostname)) == NULL){
		perror("Error when getting hostname"); 
		close(sockfd_client); 
		return; 
	}
	servaddr.sin_family = AF_INET; 
	servaddr.sin_port = htons((unsigned short)ne_udp_port);
	bcopy((char * ) hp->h_addr, (char *) &servaddr.sin_addr.s_addr, hp->h_length); 	
	
	// convert router id from host representation to network representation
	unsigned int router_id_tmp = (unsigned int) htonl(router_id); 
		
	// Send the router id to network emulator which is acting as the server
	sendto(sockfd_client, &router_id_tmp, sizeof(unsigned int), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr)); 

	// Wait for init response packet 
	memset(buffer, 0, PACKETSIZE); 
	len = sizeof(cliaddr); 
	n = recvfrom(sockfd_client, (char *) buffer, (size_t) PACKETSIZE, MSG_WAITALL, (struct sockaddr *) &cliaddr, (socklen_t *) &len); 
	
	// Create a pkt_INIT_RESPONSE structure from message and then udpate router table
	create_pkt_INIT_RESPONSE(buffer, n, &pkt_init_response); 	
	InitRoutingTbl(&pkt_init_response, router_id);  
	
	// close the client's file descriptor and then return
	close(sockfd_client); 
	return; 
}


// sets up a udp server listening on passed in port and all hostnames
int open_fd_udp(int port){
  int listenfd, optval=1;
  struct sockaddr_in serveraddr;

  /* Create a socket descriptor */
  if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
    perror("Error in trying to create UDP socket.\r\n"); 
    return -1;
  }

  /* Eliminates "Address already in use" error from bind. */
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int)) < 0){
    return -1;
  }

  /* Listenfd will be an endpoint for all requests to port on any IP address for this host */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)port);
  if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0){
    perror("Error in trying to bind udp socket.\r\n"); 
    return -1;
  }

  return listenfd;
}
