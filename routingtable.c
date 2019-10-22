#include "ne.h"
#include "router.h"


/* ----- GLOBAL VARIABLES ----- */
struct route_entry routingTable[MAX_ROUTERS];
int NumRoutes;

/*************************************************************************************** Function declarations*****************************************************************************************/
/* Routine Name    : in_table
 * INPUT ARGUMENTS : 1.  int- The destination id to be checked if there is an entry in table for it 
 *                   2. int * - Pointer to an integer that the function will place the index of entry if found in table. 
				The function does not alter this value if the entry is not found in table
 *                   
 * RETURN VALUE    : bool - true:  If entry for destination exists in routers table
			    false: There is no entry in router's table for this destination
 * USAGE           : This function should be called when a user wants to check whether the router has an entry for a 
 *  		     a particular destination. 
 */
bool in_table(int dest_id, int * entry_index); 


/* Routine Name    : forced_update_rule
 * INPUT ARGUMENTS : 1. struct route_entry * - Pointer to router's entry is having the forced update rule applied to it
                     2. int - The id of sender that sent the routing table update packet. 
                     3. int - The new calculated cost using the sender's cost and the cost to get to sender from router position 
 *
 * RETURN VALUE    : void
 * USAGE           : This should be called within a routine that is performing updates on router's entry table in the case where a new routing update packet 
 *		     has been received from one of its neighbors. This funciton simply performs the forced update logic on the entry and then updates the cost 
		     accordingly. 
 */
void forced_update_rule(struct route_entry * router, int sender, int new_cost);
 
/* Routine Name    : path_vector_rule
 * INPUT ARGUMENTS : 1. struct route_entry * - Pointer to router's entry that is having path vector rule applied
                     2. struct route_entry * - Pointer to sender's entry in routing table that will be used wtih path vector rule to be applied
                     3. int - The new calculated cost using the sender's cost and the cost to get to sender from router position 
		     4. int - The id of router that is fed as a command line argument at run time
 * 
 * RETURN VALUE    : void
 * USAGE           : This should be called within a routine that is performing updates on router's entry table in the case where a new routing update packet 
  		     has been received from one of its neighbors. This funciton simply performs the path vector rule logic on the entry and then updates the cost 
 		     accordingly. 
 */
void path_vector_rule(struct route_entry * router, struct route_entry * neighbor, int new_cost, int myID); 

/*************************************************************************************** Function definitions*****************************************************************************************/
////////////////////////////////////////////////////////////////
void InitRoutingTbl (struct pkt_INIT_RESPONSE *InitResponse, int myID){
	// Check for null pointer
	if (InitResponse == NULL){
#if (DEBUG == 1) 
		printf("pkt_INIT_RESPONSE pointer was null when passed to InitRoutingTbl function.\r\n");
		return;  
#endif
	} 

	// Initialze number of routes in routing table to zero for this router
	NumRoutes = 0; 

	// Make a cyclic route for self
	routingTable[NumRoutes].dest_id = myID; 
	routingTable[NumRoutes].next_hop = myID; 
	routingTable[NumRoutes].cost = 0;
#ifdef PATHVECTOR		 
	routingTable[NumRoutes].path_len = 2;
	routingTable[NumRoutes].path[0] = myID;
	routingTable[NumRoutes++].path[1] = myID;
#endif 

	// Loop through all of router's neighbors to initialize's its routing table
	int i; 
	for (i = 0; i < InitResponse->no_nbr; i++){		
		
		// Make routing table entry for this neighbori
		routingTable[NumRoutes].dest_id = InitResponse->nbrcost[i].nbr; 
		routingTable[NumRoutes].next_hop = InitResponse->nbrcost[i].nbr; 
		routingTable[NumRoutes].cost = InitResponse->nbrcost[i].cost;
#ifdef PATHVECTOR		 
		routingTable[NumRoutes].path_len = 2;
		routingTable[NumRoutes].path[0] = myID;
		routingTable[NumRoutes++].path[1] = InitResponse->nbrcost[i].nbr;
#endif 	
	}		
	return;
}

////////////////////////////////////////////////////////////////
int UpdateRoutes(struct pkt_RT_UPDATE *RecvdUpdatePacket, int costToNbr, int myID){
	int new_cost = 0; 
	
	// Check for null pointer error
	if (RecvdUpdatePacket == NULL){
#if (DEBUG == 1)
		printf("RecvdUpdatePacket is null when passed into UpdateRoutes\r\n"); 
#endif 
	}
	
	// Check to make sure that I am actually the destination of this update packet
	if (myID != RecvdUpdatePacket->dest_id){
#if (DEBUG == 1)
		printf("Should not be calling UpdateRoutes with a pkt_RT_UPDATE that is not addressed to me. The packet is addressed to %d\r\n", RecvdUpdatePacket->dest_id); 
#endif 
	}

	// Go through the received routing table
	int i; 
	int entry_index = 0; 
	for (i = 0; i < RecvdUpdatePacket->no_routes; i++){
		// Calculate new cost for this entry 
		if (RecvdUpdatePacket->route[i].cost != INFINITY){
			new_cost = costToNbr + RecvdUpdatePacket->route[i].cost; 
		}
		else{
			new_cost = INFINITY; 
		}
			
		
		// if not in routers table, then add it
		if (in_table(RecvdUpdatePacket->route[i].dest_id, &entry_index) == false){
			routingTable[NumRoutes].dest_id = RecvdUpdatePacket->route[i].dest_id; 
			routingTable[NumRoutes].next_hop = RecvdUpdatePacket->sender_id;
			routingTable[NumRoutes].cost = new_cost;
#ifdef PATHVECTOR 
			// Copy over path 
			routingTable[NumRoutes].path_len = RecvdUpdatePacket->route[i].path_len + 1;
			routingTable[NumRoutes].path[0] = myID; 
			int j; 
			for (j = 1; j < routingTable[NumRoutes].path_len; j++){
				routingTable[NumRoutes].path[j] = RecvdUpdatePacket->route[i].path[j-1];  
			}		
#endif 
			NumRoutes++; 
		}
		// else perform the path vector update algorithm because entry was found 
		else{
			// Forced update and path vector rules being applied
			forced_update_rule(&routingTable[entry_index], RecvdUpdatePacket->sender_id, new_cost); 
			path_vector_rule(&routingTable[entry_index], &RecvdUpdatePacket->route[i], new_cost, myID); 
		}
	}	
	
	return 0;
}

////////////////////////////////////////////////////////////////
void ConvertTabletoPkt(struct pkt_RT_UPDATE *UpdatePacketToSend, int myID){
	// Check to make sure that pkt_RT_UPDATE pointer is not null
	if (UpdatePacketToSend == NULL){
#if (DEBUG == 1)
		printf("UpdatePacketToSend is null when passed into function ConvertTablePkt\r\n"); 
#endif 		
		return; 
	}

	// Fill in sender id and number of routes but leave dest id unfilled.
	UpdatePacketToSend->sender_id = myID;
	UpdatePacketToSend->no_routes = NumRoutes;
	
	// Copy router table
	int i; 
	for (i = 0; i < NumRoutes; i++){
		UpdatePacketToSend->route[i] = routingTable[i]; 
	}	
	return;
}

/* Routine Name    : UninstallRoutesOnNbrDeath
 * INPUT ARGUMENTS : 1. int - The id of the inactive neighbor 
 *                   (one who didn't send Route Update for FAILURE_DETECTION seconds).
 *                   
 * RETURN VALUE    : void
 * USAGE           : This function is invoked when a nbr is found to be dead. The function checks all routes that
 *                   use this nbr as next hop, and changes the cost to INFINITY.
 */
////////////////////////////////////////////////////////////////
void UninstallRoutesOnNbrDeath(int DeadNbr){
	// Go through routing table and check all routes that use neighbor as next hop
	int i; 
	for (i = 0; i < NumRoutes; i++){
		if (routingTable[i].next_hop == DeadNbr){
			routingTable[i].cost = INFINITY; 
		}
	} 
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
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool in_table(int dest_id, int * entry_index){
	// Loop through router's table to check for destination entry
	int i; 
	for (i = 0; i < NumRoutes; i++){
		if ( routingTable[i].dest_id == dest_id){
			*entry_index = i; 
			return true; 
		}
	}
	return false;
}

 
void forced_update_rule(struct route_entry * router, int sender, int new_cost){
	// if neighbor is next hop for router, then it must update its entry
	if (router->next_hop == sender){
		router->cost = new_cost; 
	}
	return; 
} 

void path_vector_rule(struct route_entry * router, struct route_entry * neighbor, int new_cost, int myID){
	// Don't do anything if router is in neighbor's path to destination
#ifdef PATHVECTOR
	int i;

	for (i = 0; i < neighbor->path_len; i++){
		if (neighbor->path[i] == myID){
			return; 
		}; 
	}

	// router is not in neighbors path, so use neighbors path if it is better
	if (new_cost < router->cost){
		// Update the next_hop, path length, and cost
		router->next_hop = neighbor->path[0]; 
		router->cost = new_cost; 
		router->path_len = neighbor->path_len + 1; 

		// Update routers path
		for (i = 1; i < router->path_len; i++){
			router->path[i] = neighbor->path[i-1]; 
		}
	}
#endif 
	return; 
}

