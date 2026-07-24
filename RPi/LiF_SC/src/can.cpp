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

	// Initialize an opened CAN 2.0 channel with a 125kbps bitrate, accepting standard frames
    this->status = CAN_Init(this->h2, CAN_BAUD_125K, CAN_INIT_TYPE_ST);
    
    this->status = CAN_Status(this->h2);

	return this->status;
}

int CAN::pcanClose() {
	// Close CAN 2.0 channel and exit	
    return CAN_Close(h2);
}

int CAN::pcanRxState(TPCANMsg *msg) {
    // Clear the channel - new - Must clear the channel before Tx/Rx
	status = CAN_Status(h2);
    if (status < 0) {
        std::cerr << "CAN: error occurred while receiving the message. " \
        "Status: " << std::hex << status << std::dec << std::endl;
        return -1;
    }

    // attempt to read the message
    status = CAN_Read(h2, msg);
    
    // No message received
    if (status == PCAN_RECEIVE_QUEUE_EMPTY) {
        return 0;   // nothing was received
    }

	//Message received and valid
    if (status != PCAN_NO_ERROR) {
        std::cerr << "CAN: received message is invalid. " \
        "Status: " << std::hex << status << std::dec << std::endl;
        return -1;
    }

    // at this point, message is received and checked to be valid
    return 1;
}

int CAN::pcanTx(int id, int data) {
	// Clear the channel - new - Must clear the channel before Tx/Rx
	status = CAN_Status(h2);
    if (status < 0) {
        std::cerr << "CAN: received message is invalid. " \
        "Status: " << std::hex << status << std::dec << std::endl;
        return -1;
    }

	// Set up message
    TPCANMsg Txmsg;
	Txmsg.ID = id; 	
	Txmsg.MSGTYPE = MSGTYPE_STANDARD; 
	Txmsg.LEN = 1; 
	Txmsg.DATA[0] = data; 

    // not sure why the delay was here. trying to remove it
	// sleep(1);  
    
	status = CAN_Write(h2, &Txmsg);
    if (status < 0) {
        std::cerr << "CAN: error occurred while transmitting a message. " \
        "MSG ID: " << std::hex << id << " Data: " << data << " Status: " << \
        status << std::dec << std::endl;
        return -1;
    }

    return status;
}

CAN::CAN() {
    if (pcanInit()) {
        std::cerr << "Unable to initialize PCAN" << std::endl;
    }
}

CAN::~CAN() {
    pcanClose();
}
