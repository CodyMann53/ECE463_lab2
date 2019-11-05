#include "router.h" 
#include <pthread.h>
#include <time.h>

#define ROUTER_ID_ARGV_POSITION 1
#define NE_HOSTNAME_ARGV_POSITION 2
#define NE_UDP_PORT_ARGV_POSITION 3
#define ROUTER_UDP_PORT_ARGV_POSITION 4

#define ROUTER_ALIVE 0
#define DEAD_ROUTER_FOUND 1
#define ROUTER_SET_TO_DEAD_IN_TABLE 2

#define ROUTER_CONVERGING 0
#define ROUTER_CONVERGENCE_DETECTED 1
#define ROUTER_IN_CONVERGENCE_STATE 2

////////////////// structs /////////////////////////
struct neighbor{
	// neighbor id
	unsigned int nbr;
	// cost to neighbor 
	unsigned cost; 	
	// received update timer zero base 
	time_t update_timer_zero_base; 
	// TODO maybe add a dead router flag here  
	int router_dead_flag;  
};


struct my_neighbor_info{
	unsigned int no_nbr; 
	struct neighbor neighbors[MAX_ROUTERS]; 
}; 


struct args{
	int udp_port;
	int ne_udp_port; 
	int router_id;
	int sockfd;  
	char ne_hostname[PACKETSIZE]; 
};  

//////////////////////// Global variables////////////////////////
pthread_t udp_server_tid; 
pthread_t time_keeper_tid; 
pthread_mutex_t lock; 
time_t program_base_run_time; 


///////////////////////////// Shared memory (should use locking to access any of these variables) ///////////////////////////
struct my_neighbor_info neighbor_table; 
int update_interval_flag = 0; 
int converged_flag = 0; 
time_t convergence_timer = 0; 
FILE * fp; 

// sets up a udp server listening on passed in port and all hostnames
int open_fd_udp(int port);


void fill_neighbor_information(struct pkt_INIT_RESPONSE * pkt_init, struct my_neighbor_info * table); 


void * udp_server_thread(void * param); 


void send_update_packets(struct pkt_RT_UPDATE *UpdatePacketToSend, struct sockaddr_in * ne_addr, int fd); 


int get_cost_to_neighbor(int nbr_id); 


void signal_received_update_from_neighbor(int nbr_id); 


void * time_keeper_thread(void * param); 


void init_router(int router_id, int ne_udp_port, int router_udp_port, char * ne_hostname, int sockfd); 


void print_pkt_UPDATE(struct pkt_RT_UPDATE * pkt); 


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
	
	// Set up arguments for threads 
	struct args arguments; 
	bzero(&arguments, sizeof(struct args)); 
	arguments.udp_port = router_udp_port;
	arguments.ne_udp_port = ne_udp_port;
	arguments.router_id = router_id;  
	strcpy(arguments.ne_hostname, argv[NE_HOSTNAME_ARGV_POSITION]);  	
	arguments.sockfd = open_fd_udp(arguments.udp_port); 
	
	// Initialize the router 
	init_router(router_id, ne_udp_port, router_udp_port, argv[NE_HOSTNAME_ARGV_POSITION], arguments.sockfd); 

	// Open file and print initial table 
	char file_name[100]; 
	bzero(file_name, 100); 
	sprintf(file_name, "router_%d.log", router_id); 	
	fp = fopen(file_name, "w"); 
	PrintRoutes(fp, router_id);

	// Create the udp server and time keeper thread
	pthread_attr_t attr; 
	pthread_attr_init(&attr); 
	pthread_create(&udp_server_tid, &attr, udp_server_thread,  &arguments);  
	pthread_create(&time_keeper_tid, &attr, time_keeper_thread, NULL); 

	// Get the network emulator's address 
	if ((hp = gethostbyname(arguments.ne_hostname)) == NULL ){ 
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
		// Check on update interval timer
		pthread_mutex_lock(&lock); 
		if (update_interval_flag == 1){

			// for every neighbor, send an update packet
			int i; 
			for (i = 0; i < neighbor_table.no_nbr; i++){
				// Create an update packet and then send it to all neighbors 
				bzero(&pkt, sizeof(pkt)); 
				ConvertTabletoPkt(&pkt, arguments.router_id);
				
				// set the destination, and then send message to network emulator 				
				pkt.dest_id = neighbor_table.neighbors[i].nbr; 	
				hton_pkt_RT_UPDATE(&pkt);
				sendto(arguments.sockfd, &pkt, sizeof(struct pkt_RT_UPDATE), MSG_CONFIRM, (const struct sockaddr * ) &ne_addr, sizeof(ne_addr)); 
			}
			// Reset the update interval timer
			update_interval_flag = 0; 

		}
		pthread_mutex_unlock(&lock); 

		// Check on convergence
		pthread_mutex_lock(&lock); 
		if ( converged_flag == ROUTER_CONVERGENCE_DETECTED){
			converged_flag = ROUTER_IN_CONVERGENCE_STATE; 
#if DEBUG == 1
			printf("Routing table converged.\r\n");
#endif 
			fprintf(fp, "%lld:Converged\n", (long long) (time(NULL) - program_base_run_time)); 
			fflush(fp);
		}	
		pthread_mutex_unlock(&lock); 
	
		// check on any dead routers and update table if so
		pthread_mutex_lock(&lock); 
		int j; 
		for (j = 0; j < neighbor_table.no_nbr; j++){
			if (neighbor_table.neighbors[j].router_dead_flag == DEAD_ROUTER_FOUND){
				// Update table to signify dead neighbor
				neighbor_table.neighbors[j].router_dead_flag = ROUTER_SET_TO_DEAD_IN_TABLE;
				UninstallRoutesOnNbrDeath(neighbor_table.neighbors[j].nbr);	
				
				// reset convergence flags and timer, because the table is no longer converged
				convergence_timer = time(NULL); 
				converged_flag = ROUTER_CONVERGING; 
				
				// print updated routing table
				PrintRoutes(fp, router_id);
#if DEBUG == 1			
				printf("Dead router %d found.\r\n", neighbor_table.neighbors[j].nbr); 
#endif
			}
		}
		pthread_mutex_unlock(&lock); 
	}

	// Join threads and close socket file descriptor 
	pthread_join(udp_server_tid, NULL); 
	pthread_join(time_keeper_tid, NULL);  
	close(arguments.sockfd);  
	fclose(fp);  
	return 0; 
}

void * udp_server_thread(void * param){
	struct sockaddr_in cli_addr; 	
	int len; 
	struct pkt_RT_UPDATE recv_update_pkt; 
	int nbr_cost = 0; 
	// Retrieve passed in arguments
	struct args * arguments  = (struct args *) param;

	// Zero out buffers and structures 
	memset(&cli_addr, 0, sizeof(cli_addr)); 

	// Start receiving incoming router update messages		
	while (1){
		// Wait for a router update packet 
		bzero(&recv_update_pkt, sizeof(struct pkt_RT_UPDATE)); 
		len = sizeof(cli_addr); 
		recvfrom(arguments->sockfd, &recv_update_pkt, sizeof(struct pkt_RT_UPDATE), MSG_WAITALL, (struct sockaddr *) &cli_addr, (socklen_t *) &len);
		ntoh_pkt_RT_UPDATE(&recv_update_pkt);
		
		// Update router table and signal that recieved an update message from this neighbor
		pthread_mutex_lock(&lock); 
		nbr_cost = get_cost_to_neighbor(recv_update_pkt.sender_id);
		signal_received_update_from_neighbor(recv_update_pkt.sender_id); 
		
		// If router table changed, then reset convergence timer and signal that still converging
		if (UpdateRoutes(&recv_update_pkt, nbr_cost, arguments->router_id) == 1){
			convergence_timer = time(NULL); 
			converged_flag = ROUTER_CONVERGING; 
			PrintRoutes(fp, arguments->router_id);
		}
		
		pthread_mutex_unlock(&lock); 
	}

	// Close socket and return from thread 
	pthread_exit(0);
}

void signal_received_update_from_neighbor(int nbr_id){
	int i;
	
	// Look for neighbor in table and reset its update timer to a new zero base and reset router dead flag
	for (i = 0; i < neighbor_table.no_nbr; i++){
		if (nbr_id == neighbor_table.neighbors[i].nbr){
			neighbor_table.neighbors[i].update_timer_zero_base = time(NULL); 
			neighbor_table.neighbors[i].router_dead_flag = 0; 
			return; 
		}
	}

#if DEBUG == 1	
	printf("Could not find neighbor in table when signaling that received update from neighbor R%d/r/n", nbr_id);
	
#endif
	return;  
}
int get_cost_to_neighbor(int nbr_id){
	int cost = 0; 
	int i;
	
	// Look for neighbor in table 
	for (i = 0; i < neighbor_table.no_nbr; i++){
		if (nbr_id == neighbor_table.neighbors[i].nbr){
			cost = neighbor_table.neighbors[i].cost; 
			break; 
		}
	}

#if DEBUG == 1	
	if (cost == 0){
		printf("Could not find neighbor in table when checking for R%d  cost./r/n", nbr_id);
	}
#endif
	return cost;  
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
		
		// Check for convergence 
		pthread_mutex_lock(&lock); 	
		if ( ((time(NULL) - convergence_timer) > CONVERGE_TIMEOUT) & (converged_flag != ROUTER_IN_CONVERGENCE_STATE) ){
			converged_flag = ROUTER_CONVERGENCE_DETECTED; 
		}
		pthread_mutex_unlock(&lock); 	
		

		// Check on any dead routers 
		pthread_mutex_lock(&lock); 	
		int i; 
		for (i = 0; i < neighbor_table.no_nbr; i++){
			if (((time(NULL) - neighbor_table.neighbors[i].update_timer_zero_base) > FAILURE_DETECTION) & (neighbor_table.neighbors[i].router_dead_flag == ROUTER_ALIVE)){
				neighbor_table.neighbors[i].router_dead_flag = DEAD_ROUTER_FOUND;
			}
		}
		pthread_mutex_unlock(&lock); 	
	}
	pthread_exit(0);
}


void print_pkt_UPDATE(struct pkt_RT_UPDATE * pkt){
	printf("---------RT_UPDATE packet--------\r\n"); 
	printf("Sender id: %d\r\n", pkt->sender_id); 
	printf("Dest id: %d\r\n", pkt->dest_id); 
	printf("no_routes: %d\r\n", pkt->no_routes);
	
	// Go through the route entries
	int i;
	int j;
	for(i = 0; i < pkt->no_routes; i++){
		printf("<R%d -> R%d> Path: R%d", pkt->sender_id, pkt->route[i].dest_id, pkt->sender_id);
		/* ----- PRINT PATH VECTOR ----- */
		for(j = 1; j < pkt->route[i].path_len; j++){
			printf(" -> R%d", pkt->route[i].path[j]);	
		}
		printf(", Cost: %d\n", pkt->route[i].cost);
	}
	printf("\n"); 
	return; 
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


void init_router(int router_id, int ne_udp_port, int router_udp_port, char * ne_hostname, int sockfd){
	int len; 
	struct sockaddr_in servaddr, cliaddr; 
	struct pkt_INIT_REQUEST pkt_req; 
	struct hostent *hp;
	struct pkt_INIT_RESPONSE pkt_init_response;
	bzero(&pkt_init_response, sizeof(struct pkt_INIT_RESPONSE));  

	// Zero out sock address structs 
	memset(&servaddr, 0, sizeof(servaddr)); 
	memset(&cliaddr, 0, sizeof(cliaddr)); 

	// filling server information
	if ((hp = gethostbyname(ne_hostname)) == NULL){
		perror("Error when getting hostname"); 
		return; 
	}
	servaddr.sin_family = AF_INET; 
	servaddr.sin_port = htons((unsigned short)ne_udp_port);
	bcopy((char * ) hp->h_addr, (char *) &servaddr.sin_addr.s_addr, hp->h_length); 	

	// convert router id from host representation to network representation
	pkt_req.router_id = (unsigned int) htonl(router_id); 

	// Send the router id to network emulator which is acting as the server
	sendto(sockfd, &pkt_req, sizeof(struct pkt_INIT_REQUEST), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr)); 

	// Wait for init response packet and then update table as well as start the router's time base
	len = sizeof(cliaddr); 
	recvfrom(sockfd, &pkt_init_response, sizeof(struct pkt_INIT_RESPONSE), MSG_WAITALL, (struct sockaddr *) &cliaddr, (socklen_t *) &len); 
	program_base_run_time = time(NULL); 
	ntoh_pkt_INIT_RESPONSE(&pkt_init_response);
	InitRoutingTbl(&pkt_init_response, router_id);  
	
	// Fill out neighbor information
	fill_neighbor_information(&pkt_init_response, &neighbor_table); 
	
	// Start convergence timer		
	convergence_timer = time(NULL); 
	converged_flag = ROUTER_CONVERGING; 	
	
	// Set all neighbors to alive	
	int j; 
	for (j = 0; j < neighbor_table.no_nbr; j++){
		neighbor_table.neighbors[j].router_dead_flag = ROUTER_ALIVE;
	}
	
	return; 
}


void fill_neighbor_information(struct pkt_INIT_RESPONSE * pkt_init, struct my_neighbor_info * table){
	// fill up neighbor table based on init response from network emulator
	bzero(table, sizeof(struct my_neighbor_info)); 
	table->no_nbr = pkt_init->no_nbr; 	
	int i; 
	for (i = 0; i < pkt_init->no_nbr; i++){
		table->neighbors[i].nbr = pkt_init->nbrcost[i].nbr; 
		table->neighbors[i].cost = pkt_init->nbrcost[i].cost; 
		table->neighbors[i].update_timer_zero_base = time(NULL); 
	}	
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
