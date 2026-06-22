#include "../include/pcanFunctions.h"
#include "../include/databaseFunctions.h"
#include "../include/mainFunctions.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <iostream>
#include <time.h> // for timeout

using namespace std;
State currentState = STATE_IDLE;

// #define DO_USE_DB




// ******************************************************************

int main() {

	int choice; 
	int ID; 
	int data; 
	int numRx;
	int floorNumber = 1, prev_floorNumber = 1;

	while(1) {
		system("@cls||clear");
		choice = menu(); 
		switch (choice) {
			case 1: 
				ID = chooseID();		// user to select ID depending on intended recipient
				data = chooseMsg();		// user to select message data
				pcanTx(ID, data);		// transmit ID and data 
#ifdef DO_USE_DB
				db_setFloorNum(FloorFromHex(data)); 		// change floor number in database ** NEW **
#endif
				break; 
				
			case 2:
				printf("\nHow many messages to receive? ");
				scanf("%d", &numRx);
				pcanRx(numRx);
				break;
				
			case 3:
				printf("\nNow listening to commands from the website - press ctrl-z to cancel\n");
				
				pcanRxInit();
	
				// Synchronize elevator db and CAN (start at 1st floor)
				pcanTx(ID_SC_TO_EC, GO_TO_FLOOR1);
#ifdef DO_USE_DB
				db_setFloorNum(1);
#endif
				currentState = STATE_IDLE;
				
				{
					bool is_CC_door_closed = false;
					TPCANMsg incoming;
					int targetFloor = 1;
					//int activeTargetFloor = -1;
					time_t moveStartTime = 0;

					while(1){			

						//Check if there is a new message request, flag
						int pendingFloor = -1; //no other message in the queue
						bool newRequest = false;

						//Check database for requests
#ifdef DO_USE_DB
						floorNumber = db_getFloorNum();
#endif
						if (prev_floorNumber != floorNumber) {								// If floor number changes in database
							pendingFloor = floorNumber;
							printf("Queued floor %d\n", floorNumber);
							enqueueFloor(floorNumber);
						}

						// prev_floorNumber = floorNumber; 

						// Check CAN for floor requests
						int gotMessage = pcanRxState(&incoming);
						if (gotMessage == 1)
						{
							printf("Received CAN frame with ID %x ", incoming.ID);
							switch(incoming.ID) {
								case ID_F1_TO_SC:
									printf("- Queue FC1\n");
									enqueueFloor(1);
									break;
								case ID_F2_TO_SC:
									printf("- Queue FC2\n");
									enqueueFloor(2);
									break;
								case ID_F3_TO_SC:
									printf("- Queue FC3\n");
									enqueueFloor(3);
									break;
								case ID_CC_TO_SC:
									if ((incoming.DATA[0] & 0b00000100) > 0) {
										is_CC_door_closed = true;
										printf("- CC doors closed ");

										if ((incoming.DATA[0] & 0x03) != 0) {
											// CC_FloorReq is bits 1-0 of the data byte
											printf("- Queue CC %x\n", incoming.DATA[0] & 0x03);
											enqueueFloor(incoming.DATA[0] & 0x03);
										}

									} else {
										is_CC_door_closed = false;
										printf("- CC doors open - ignore request\n");
										
									}

									break;
								case ID_EC_TO_ALL:
									printf("EC heartbeat\n");
									break;
							}
						} 
						else if (gotMessage == -1)
						{
   						printf("WARNING: CAN bus error detected\n");
						// Will have more functions to fix the error
						}
						//FSM Logic
						switch(currentState) {

						case STATE_IDLE:
							if (queueCount > 0 && is_CC_door_closed) {
								targetFloor = dequeueFloor();
								pcanTx(ID_SC_TO_EC, HexFromFloor(targetFloor));
								moveStartTime = time(NULL);
								printf("Moving to floor %d\n", targetFloor);
								currentState = STATE_MOVING;
							}
						
							break;

						case STATE_MOVING:
	
							if ((time(NULL) - moveStartTime)
								> MOVE_TIMEOUT_SEC)
							{
								printf("ERROR: Elevator timeout\n");

								currentState = STATE_FAULT;
								break;
							}

							if (gotMessage &&
								incoming.ID == ID_EC_TO_ALL)
							{
								int reportedFloor =
									incoming.DATA[0] & 0x03;

								if (reportedFloor == targetFloor)
								{
									currentState =
										STATE_ARRIVED;
								}
							}

							break;
					

						case STATE_ARRIVED:
#ifdef DO_USE_DB
							db_setFloorNum(targetFloor);
#endif
							currentState = STATE_IDLE;
							break;

						// Error handler
						case STATE_FAULT:

							printf("\nFAULT STATE\n");
							printf("Elevator did not arrive within timeout\n");

							while(queueCount > 0)
							{
								dequeueFloor();
							}

							sleep(3);

							currentState = STATE_IDLE;

							break;
						}
					}
					pcanRxClose();
				}
				break;
				
			case 4: 
				return(0);
			
			default:
				printf("Error on input values");
				sleep(3);
				break;
		}
		sleep(1);					// delay between send/receive
	}
	
	return(0);
}






	
