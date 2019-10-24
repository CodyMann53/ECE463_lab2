#include "router.h" 

#define ROUTER_ID_ARGV_POSITION 1
#define NE_HOSTNAME_ARGV_POSITION 2
#define NE_UDP_PORT_ARGV_POSITION 3
#define ROUTER_UDP_PORT_ARGV_POSITION 4

// sets up a udp server listening on passed in port and all hostnames
int open_fd_udp(int port);
 
void init_router(int router_id, int ne_udp_port, int router_udp_port, char * ne_hostname); 

int main (int argc, char ** argv){
	// Convert a port number command line args to integers
	int router_id = atoi(argv[ROUTER_ID_ARGV_POSITION]); 
	int ne_udp_port = atoi(argv[NE_UDP_PORT_ARGV_POSITION]);
	int router_udp_port = atoi(argv[ROUTER_UDP_PORT_ARGV_POSITION]); 

	// Initialize the router 
	init_router(router_id, ne_udp_port, router_udp_port, argv[NE_HOSTNAME_ARGV_POSITION]); 


	return 0; 
}

void init_router(int router_id, int ne_udp_port, int router_udp_port, char * ne_hostname){
	int sockfd_client; 
	char buffer[PACKETSIZE];
	int n, len; 
	struct sockaddr_in servaddr, cliaddr; 
  	struct hostent *hp;

	// creating socket file descriptor
	if ( (sockfd_client = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("socket creation failed"); 
		exit(EXIT_FAILURE); 
	}
	
	memset(&servaddr, 0, sizeof(servaddr)); 
	memset(&cliaddr, 0, sizeof(cliaddr)); 
	
	// filling server information
	servaddr.sin_family = AF_INET; 
	servaddr.sin_port = ne_udp_port;
	//inet_aton(gethostbyname(ne_hostname), &servaddr.sin_addr.s_addr); 	
	
/*	// convert router id from host representation to network representation
	router_id = (uint32_t) htonl(router_id); 
		
	// Send the router id to network emulator which is acting as the server
	sendto(sockfd_client, &router_id, sizeof(uint32_t), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr)); 

	// receive response back and then print for debugging purposes
	memset(buffer, 0, PACKETSIZE); 
	n = recvfrom(sockfd_client, (char *) buffer, PACKETSIZE, MSG_WAITALL, (struct sockaddr *) &cliaddr, &len); 
	print("Response: %s\r\n", buffer);  
*/	
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
