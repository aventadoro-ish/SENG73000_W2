#include "can.h"
#include <iostream>
#include "setting.h"


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
    h2 = LINUX_CAN_Open(pcan_resource_path, O_RDWR);
    if (h2 == nullptr) {
        std::cerr << "CAN: Unable to open resource on path: " << \
        pcan_resource_path << "Status: " << std::hex << status << \
        std::dec << std::endl;
        return -1;
    }

	// Initialize an opened CAN 2.0 channel with a 125kbps bitrate, accepting standard frames
    status = CAN_Init(h2, CAN_BAUD_125K, CAN_INIT_TYPE_ST);

    // ignore no error status and empty rx queue status
    if ((status != CAN_ERR_OK) && (status != CAN_ERR_QRCVEMPTY)) {
        std::cerr << "CAN: Unable to initialize CAN. " \
        "Status: " << std::hex << status << std::dec << std::endl;
        return -1;
    }
    status = CAN_Status(h2);
    // ignore no error status and empty rx queue status
    if ((status != CAN_ERR_OK) && (status != CAN_ERR_QRCVEMPTY)) {
        std::cerr << "CAN: error after initializing CAN. " \
        "Status: " << std::hex << status << std::dec << std::endl;
        return -1;
    }

	return status;
}

int CAN::pcanClose() {
	// Close CAN 2.0 channel and exit	
    return CAN_Close(h2);
}

int CAN::pcanRxState(TPCANMsg *msg) {
    // Clear the channel - new - Must clear the channel before Tx/Rx
	status = CAN_Status(h2);
    if (status != CAN_ERR_OK) {
        std::cerr << "CAN: error occurred while receiving the message. " \
        "Status: " << std::hex << status << std::dec << std::endl;
        return -1;
    }

    // attempt to read the message
    status = CAN_Read(h2, msg);
    
    // No message received
    if (status == CAN_ERR_QRCVEMPTY) {
        return 0;   // nothing was received
    }

	//Message received and valid
    if (status != CAN_ERR_OK) {
        std::cerr << "CAN: received message is invalid. " \
        "Status: " << std::hex << status << std::dec << std::endl;
        return -1;
    }

    // at this point, message is received and checked to be valid
    return 1;
}

// CAN::ID CAN::intToID(int int_id) {
//     ID id = static_cast<ID>(int_id);
//     switch (id) {
//     case ID::SC_EC_FLOOR_RQ:
//         break;
//     case ID::EC_ALL_STATUS:
//         break;
//     case ID::CC_SC_FLOOR_RQ:
//         break;
//     case ID::SC_CC_VIRT_DOOR:
//         break;
//     case ID::F1_RQ:
//         break;
//     case ID::F2_RQ:
//         break;
//     case ID::F3_RQ:
//         break;
//     case ID::UNKNOWN:
//     default:
//         return   ID::UNKNOWN;
//     }
// }

int CAN::pcanTx(int id, int data) {
	// Clear the channel - new - Must clear the channel before Tx/Rx
	status = CAN_Status(h2);
    // ignore no error status and empty rx queue status
    if ((status != CAN_ERR_OK) && (status != CAN_ERR_QRCVEMPTY)) {
        std::cerr << "CAN: bad status before transmitting. " \
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
    if (status != CAN_ERR_OK) {
        std::cerr << "CAN: error occurred while transmitting a message. " \
        "MSG ID: " << std::hex << id << " Data: " << data << " Status: " << \
        status << std::dec << std::endl;
        return -1;
    }

    return status;
}

int CAN::pcanTx(ID id, int data) {
    return pcanTx(static_cast<int>(id), data);
}

void CAN::ec_go_to_floor(unsigned int floor, bool ec_enable) {
    if (floor > NUM_FLOORS) {
        std::cerr << "Illegal floor: " << floor << ", max is " << NUM_FLOORS << std::endl;
        return;
    }

    int data = \
        (static_cast<int>(ec_enable) << 2) |
        (floor & 0b11); // floor bits
    
    pcanTx(CAN::ID::SC_EC_FLOOR_RQ, data);
}

void CAN::cc_set_doors(bool is_open) {
    if (is_open) {
        pcanTx(ID::SC_CC_VIRT_DOOR, 1);    
    } else {
        pcanTx(ID::SC_CC_VIRT_DOOR, 0);    
    }
}

int CAN::rx_can_frame(RxFrame *rx_buffer) {
    TPCANMsg msg;

    int res = pcanRxState(&msg);
    if (res != 1) {
        return res;
    }

    // convert id to CAN::ID and determine message type
    ID id = static_cast<ID>(msg.ID);
    rx_buffer->id = id;     // same operation for all cases
                            // only data decoding logic changes between messages
    switch (id) {
    case ID::EC_ALL_STATUS: {
        EC_Status ec_stat;
        ec_stat.is_enabled =    (msg.DATA[0] & 0b00000100) > 0;
        ec_stat.is_moving =     (msg.DATA[0] & 0b00001000) > 0;
        ec_stat.position =      (msg.DATA[0] & 0b00000011);  
        rx_buffer->data.ec_status = ec_stat;
        break;
    }
    case ID::CC_SC_FLOOR_RQ: {
        CC_Request rq;
        rq.is_door_open =       (msg.DATA[0] & 0b00000100) > 0;
        rq.floor_request =      (msg.DATA[0] & 0b00000011);
        rx_buffer->data.cc_request = rq;
        break;
    }
    case ID::F1_RQ:         [[fallthrough]]; // Fx_RQ messages can be treated the same way
    case ID::F2_RQ:         [[fallthrough]];
    case ID::F3_RQ: {
        Fx_Request rq;
        rq.is_requested =       (msg.DATA[0] & 0b00000001) > 0;
        rx_buffer->data.fx_request = rq;
        break;
    }    
    case ID::SC_CC_VIRT_DOOR:       [[fallthrough]];
    case ID::SC_EC_FLOOR_RQ:        
        // SC isn't expected to receive its own messages
        // assume error has occurred and treat as an unknown frame
        std::cerr << "Warning! SC received a messaged with its own ID: 0x" << \
            std::hex << msg.ID << std::dec << std::endl;   
        [[fallthrough]];
    case ID::UNKNOWN:               [[fallthrough]];
    default:
        rx_buffer->id = ID::UNKNOWN;    // make this the same for any unknown id
        // unknown CAN frame id
        for (int i = 0; i < msg.LEN; i++) {
            rx_buffer->data.unknown.data[i] = msg.DATA[i];
        }
        rx_buffer->data.unknown.dlc = msg.LEN;
        rx_buffer->data.unknown.id  = msg.ID;
        break;
    }
    
    return res;
}

CAN::CAN() {
    pcanInit();
    if ((status != CAN_ERR_OK) && (status != CAN_ERR_QRCVEMPTY)) {
        std::cerr << "Unable to initialize PCAN" << std::endl;
    }
}

CAN::~CAN() {
    pcanClose();
}
