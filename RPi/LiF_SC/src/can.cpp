#include "can.h"
#include <iostream>


#if (defined (_WIN32) || defined (_WIN64))
    // windows code
    int sleep(int ms) { 
        ; // not designed to run on Windows
    }
#elif (defined (LINUX) || defined (__linux__))
#include <fcntl.h>    					// O_RDWR
#include <unistd.h>
#else
#error "Unsupported OS"
#endif


constexpr unsigned int PCAN_RECEIVE_QUEUE_EMPTY = 0x00020U;  	// Receive queue is empty
constexpr unsigned int PCAN_NO_ERROR            = 0x00000U;  	// No error 



int CAN::pcanInit() {
    this->h2 = LINUX_CAN_Open(pcan_resource_path, O_RDWR);
    this->status = CAN_Init(this->h2, CAN_BAUD_125K, CAN_INIT_TYPE_ST);
    this->status = CAN_Status(this->h2);

	return this->status;
}

int CAN::pcanClose() {
    return CAN_Close(h2);
}

int CAN::pcanRxState(TPCANMsg *msg) {
	//Message received adn valid
    if (status == PCAN_NO_ERROR) {
        return 1;
    }
	// No message, waiting
    if (status == PCAN_RECEIVE_QUEUE_EMPTY) {
        return 0;
    }
    // anything else is a real error - rx. hardware disconnected
    printf("CAN Rx error: 0x%x\n", (int)status);
    return -1;
}

int CAN::pcanTx(int id, int data) {
	// Initialize an opened CAN 2.0 channel with a 125kbps bitrate, accepting standard frames
	status = CAN_Init(h2, CAN_BAUD_125K, CAN_INIT_TYPE_ST);

	// Clear the channel - new - Must clear the channel before Tx/Rx
	status = CAN_Status(h2);

	// Set up message
	Txmsg.ID = id; 	
	Txmsg.MSGTYPE = MSGTYPE_STANDARD; 
	Txmsg.LEN = 1; 
	Txmsg.DATA[0] = data; 

	sleep(1);  
	status = CAN_Write(h2, &Txmsg);

	// Close CAN 2.0 channel and exit	
	CAN_Close(h2);
    return status;
}

int CAN::pcanRx(int num_msgs) {
	int i = 0;

	// Open a CAN channel 
	h2 = LINUX_CAN_Open(pcan_resource_path, O_RDWR);

	// Initialize an opened CAN 2.0 channel with a 125kbps bitrate, accepting standard frames
	status = CAN_Init(h2, CAN_BAUD_125K, CAN_INIT_TYPE_ST);

	// Clear the channel - new - Must clear the channel before Tx/Rx
	status = CAN_Status(h2);

	// Clear screen to show received messages
	system("@cls||clear");

	// receive CAN message  - CODE adapted from PCAN BASIC C++ examples pcanread.cpp
	printf("\nReady to receive message(s) over CAN bus\n");
	
	// Read 'num' messages on the CAN bus
	while(i < num_msgs) {
		while((status = CAN_Read(h2, &Rxmsg)) == PCAN_RECEIVE_QUEUE_EMPTY){
			sleep(1);
		}
		if(status != PCAN_NO_ERROR) {						// If there is an error, display the code
			printf("Error 0x%x\n", (int)status);
			//break;
		}
										
		if(Rxmsg.ID != 0x01 && Rxmsg.LEN != 0x04) {		// Ignore status message on bus	
			printf("  - R ID:%4x LEN:%1x DATA:%02x \n",	// Display the CAN message
				(int)Rxmsg.ID, 
				(int)Rxmsg.LEN,
				(int)Rxmsg.DATA[0]);
		i++;
		}
	}
	

	// Close CAN 2.0 channel and exit	
	CAN_Close(h2);
	//printf("\nEnd Rx\n");
	return ((int)Rxmsg.DATA[0]);						// Return the last value received
}
