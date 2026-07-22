#include "../include/pcanFunctions.h"
#include "../include/databaseFunctions.h"
#include "../include/mainFunctions.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <iostream>
#include <time.h> // for timeout

using namespace std;

// -----------------------------------------------------------------------------
// Settings
// -----------------------------------------------------------------------------

// #define DO_USE_DB
// #define DO_USE_CAN

constexpr unsigned int NUM_FLOORS = 3;

/**
 * @brief How long to wait between moving to the next floor in sabbath mode 
 * (in milliseconds)
 */
constexpr unsigned long int SABBATH_MOVE_DELAY_MS = 5000;

/**
 * @brief How long to wait on a floor if other floor requests are queued in 
 * normal mode (in milliseconds)
 */
constexpr unsigned long int FLOOR_WAIT_DELAY_MS = 3000;

/**
 * @brief Where to send the cabin during initialization
 */
constexpr unsigned int INITIAL_FLOOR = 1;


// -----------------------------------------------------------------------------
// Global Declarations
// -----------------------------------------------------------------------------

/**
 * @brief States of supervisory controller FSM
 */
enum class SC_State : unsigned int {
	INITIALIZE,
	IDLE,
	MOVING,
	SABBATH_IDLE,
	SABBATH_MOVING,
	MAINTENANCE,
	MAINTENANCE_MOVING,
	FAULT
};


// -----------------------------------------------------------------------------
// Global variables
// -----------------------------------------------------------------------------
bool is_running = true;
SC_State state = SC_State::INITIALIZE;

int floorNumber = INITIAL_FLOOR;
int prev_floorNumber = INITIAL_FLOOR;
bool is_CC_door_closed = false;
int pendingFloor = -1; //no other message in the queue



// -----------------------------------------------------------------------------
// Function declarations
// -----------------------------------------------------------------------------
void FSM_normal_mode();

void FSM_sabbath_mode();

void FSM_maintenance_mode();

/**
 * @brief Starts up the CAN network and DB connection. 
 * Makes sure all nodes are present and functional and DB is up.
 */
void FSM_initialize();

/**
 * @brief Stop operations and wait for maintenance command from DB
 */
void FSM_fault_mode();


/**
 * @brief Attempt to read floor request from DB or CAN network
 * 
 * Also reads CAN messages and updates is_door_closed flag
 * @return 0 if no request was made. Number between 1 and NUM_FLOORS if a request 
 * was made.
 */
int get_floor_request();



// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------
int main() {

	int choice; 
	int ID; 
	int data; 
	int numRx;
	TPCANMsg incoming;

	
	printf("\nLithium Firefly Project. Initializing SC FSM\nProgram settings\n");
	printf(" - number of floors: %u\n", NUM_FLOORS);
	printf(" - queued floor wait time (ms): %lu\n", FLOOR_WAIT_DELAY_MS);
	printf(" - sabbath mode wait time (ms): %lu\n", SABBATH_MOVE_DELAY_MS);
	printf(" - initial floor: %u\n", INITIAL_FLOOR);


	pcanRxInit();

	// Synchronize elevator db and CAN (start at 1st floor)
	pcanTx(ID_SC_TO_EC, GO_TO_FLOOR1);
#ifdef DO_USE_DB
	db_setFloorNum(1);
#endif


	// initialize variables  
	int targetFloor = 1;
	time_t moveStartTime = 0;

	while(is_running) { // FSM infinite loop start
		

		switch (state) {
			case SC_State::INITIALIZE:
				FSM_initialize();
				break;
			case SC_State::IDLE:
			case SC_State::MOVING:
				FSM_normal_mode();
				break;
			case SC_State::SABBATH_IDLE:
			case SC_State::SABBATH_MOVING:
				FSM_sabbath_mode();
				break;
			case SC_State::MAINTENANCE:
			case SC_State::MAINTENANCE_MOVING:
				FSM_maintenance_mode();
				break;
			default:

				break;
		}


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
		}  // FSM infinite loop end
		

	}
	
	pcanRxClose();
	return 0;
}


// -----------------------------------------------------------------------------
// Function definitions
// -----------------------------------------------------------------------------
void FSM_normal_mode() {
	//Check if there is a new message request, flag
	int pendingFloor = -1; //no other message in the queue
	bool newRequest = false;



}

void FSM_sabbath_mode() {

}

void FSM_maintenance_mode() {

}

void FSM_initialize() {

}

void FSM_fault_mode() {

}


int get_floor_request() {
	TPCANMsg incoming;

	//Check database for requests
#ifdef DO_USE_DB
	floorNumber = db_getFloorNum();
#endif

	if (prev_floorNumber != floorNumber) {								// If floor number changes in database
		pendingFloor = floorNumber;
		printf(" -> Queued floor based on DB request: %d\n", floorNumber);
		return floorNumber;
	}

	// Check CAN for floor requests
	int gotMessage = pcanRxState(&incoming);
	if (gotMessage == 1)
	{
		printf(" -> Received CAN frame with ID %x ", incoming.ID);
		switch(incoming.ID) {
			case ID_F1_TO_SC:
				printf("- Queue FC1\n");
				return 1;
				break;
			case ID_F2_TO_SC:
				printf("- Queue FC2\n");
				return 2;
				break;
			case ID_F3_TO_SC:
				printf("- Queue FC3\n");
				return 3;
				break;
			case ID_CC_TO_SC:
				if ((incoming.DATA[0] & 0b00000100) > 0) {
					is_CC_door_closed = true;
					printf("- CC doors closed ");

					if ((incoming.DATA[0] & 0x03) != 0) {
						// CC_FloorReq is bits 1-0 of the data byte
						printf("- Queue CC %x\n", incoming.DATA[0] & 0x03);
						return incoming.DATA[0] & 0x03;
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

}