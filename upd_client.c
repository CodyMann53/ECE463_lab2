#include "router.h"

int open_fd_udp(int port); 

int main(int argc, char ** argv){
	socklen_t len;  
	struct hostent *hp;	
	struct pkt_RT_UPDATE pkt; 
	bzero(&pkt, sizeof(pkt)); 
	struct sockaddr_in srcaddr; 
        
		
	// Open the file udp connection
	int listenfd; 
	int port = atoi(argv[1]); 
  	/* Create a socket descriptor */
  	if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
    		perror("Error in trying to create UDP socket.\r\n"); 
    		return -1;
  	}

//	int sockfd = open_fd_udp(port); 
	
	// Listen for data
	memset(&srcaddr, 0, sizeof(srcaddr)); 
	memset(buffer, 0, PACKETSIZE); 
	len = sizeof(srcaddr); 
	//n = recvfrom(sockfd, (char *) buffer, (size_t) PACKETSIZE, MSG_WAITALL, (struct sockaddr *) &srcaddr,  &len); 
	//printf("Recieved %d bytes. Response: %s\r\n", n, buffer);  
	
	// Echo back received data 
	sendto(sockfd, buffer, n, MSG_CONFIRM, (const struct sockaddr *) &srcaddr, sizeof(srcaddr)); 
	
	// close the server's file descriptor and then return
	close(sockfd); 	
	return 0; 
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
    	perror("Error when trying to set socket options.\r\n"); 
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
	
