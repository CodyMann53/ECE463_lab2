#include "ne.h"
#include "router.h"


/* ----- GLOBAL VARIABLES ----- */
struct route_entry routingTable[MAX_ROUTERS];
int NumRoutes;


////////////////////////////////////////////////////////////////
void InitRoutingTbl (struct pkt_INIT_RESPONSE *InitResponse, int myID){
	// Check for null pointer
	if (InitResponse == NULL){
#if (DEBUG == 1) 
		printf("pkt_INIT_RESPONSE pointer was null when passed to InitRoutingTbl function.\r\n"); 
#endif
	} 

	// Initialze number of routes in routing table to zero for this router
	NumRoutes = 0; 

	// Make a cyclic route for self
	routingTable[NumRoutes].dest_id = myID; 
	routingTable[NumRoutes].next_hop = myID; 
	routingTable[NumRoutes].cost = 0;
#ifdef PATHVECTOR		 
	routingTable[NumRoutes].path_len = 1;
	routingTable[NumRoutes++].path[0] = myID;
#endif 

	// Loop through all of router's neighbors to initialize's its routing table
	for (int i = 0; i < InitResponse->no_nbr; i++){		
		
		
		// Make routing table entry for this neighbori
		routingTable[NumRoutes].dest_id = InitResponse->nbrcost[i].nbr; 
		routingTable[NumRoutes].next_hop = InitResponse->nbrcost[i].nbr; 
		routingTable[NumRoutes].cost = InitResponse->nbrcost[i].cost;
#ifdef PATHVECTOR		 
		routingTable[NumRoutes].path_len = 1;
		routingTable[NumRoutes++].path[0] = InitResponse->nbrcost[i].nbr;
#endif 	
	}		
	return;
}

////////////////////////////////////////////////////////////////
int UpdateRoutes(struct pkt_RT_UPDATE *RecvdUpdatePacket, int costToNbr, int myID){
	/* ----- YOUR CODE HERE ----- */
	return 0;
}


////////////////////////////////////////////////////////////////
void ConvertTabletoPkt(struct pkt_RT_UPDATE *UpdatePacketToSend, int myID){
	/* ----- YOUR CODE HERE ----- */
	return;
}





////////////////////////////////////////////////////////////////
void UninstallRoutesOnNbrDeath(int DeadNbr){
	/* ----- YOUR CODE HERE ----- */
	return;
}

////////////////////////////////////////////////////////////////
//It is highly recommended that you do not change this function!
void PrintRoutes (FILE* Logfile, int myID){
	/* ----- PRINT ALL ROUTES TO LOG FILE ----- */
	int i;
	int j;
	for(i = 0; i < NumRoutes; i++){
		fprintf(Logfile, "<R%d -> R%d> Path: R%d", myID, routingTable[i].dest_id, myID);

		/* ----- PRINT PATH VECTOR ----- */
		for(j = 1; j < routingTable[i].path_len; j++){
			fprintf(Logfile, " -> R%d", routingTable[i].path[j]);	
		}
		fprintf(Logfile, ", Cost: %d\n", routingTable[i].cost);
	}
	fprintf(Logfile, "\n");
	fflush(Logfile);
}
