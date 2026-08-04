
#ifdef DO_NOT_SET_THIS_FLAG
int pendingFloor = -1; //no other message in the queue

int old_fsm() {
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




/**
 * @brief Attempt to read floor request from DB or CAN network
 * 
 * Also reads CAN messages and updates is_door_closed flag
 * @return 0 if no request was made. Number between 1 and NUM_FLOORS if a request 
 * was made.
 */
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
#endif