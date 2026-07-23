#ifndef PCAN_FUNCTIONS
#define PCAN_FUNCTIONS

#ifdef DO_USE_CAN
#include <libpcan.h> // used for TPCANMsg
#else 
// define proxy for the datatypes used by the library

// compatibilty defines
#if defined(DWORD) || defined(WORD) || defined(BYTE)
#error "double define for DWORD, WORD, BYTE found"
#endif

#include <cstdint>

#define DWORD  uint32_t
#define WORD   uint16_t
#define BYTE   uint8_t

typedef struct 
{
  DWORD ID;              // 11/29 bit code
  BYTE  MSGTYPE;         // bits of MSGTYPE_*
  BYTE  LEN;             // count of data bytes (0..8)
  BYTE  DATA[8];         // data bytes, up to 8
} TPCANMsg;              // for PCAN_WRITE_MSG

#endif


// Defines
// ***********************************************************************************************************
#define PCAN_RECEIVE_QUEUE_EMPTY        0x00020U  	// Receive queue is empty
#define PCAN_NO_ERROR               	0x00000U  	// No error 

// Elevator project specific 
#define ID_SC_TO_EC  0x100	// ID for messages from Supervisory controller to elevator controller
#define ID_EC_TO_ALL 0x101	// ID for messages from Elevator controller to all other nodes
#define ID_CC_TO_SC  0x200	// ID for messages from Car controller to supervisory controller 
#define ID_F1_TO_SC  0x201	// ID for messages from floor 1 controller to supervisory controller
#define ID_F2_TO_SC  0x202	// ID for messages from floor 2 controller to supervisory controller
#define ID_F3_TO_SC  0x203	// ID for messages from floor 3 controller to supervisory controller	

#define GO_TO_FLOOR1 0x05	// Go to floor 1
#define GO_TO_FLOOR2 0x06	// Go to floor 2
#define GO_TO_FLOOR3 0x07	// Go to floor 3

//New for FSM
int pcanRxInit();
int pcanRxClose();
int pcanRxState(TPCANMsg *msg); // if a message is recieved = 1, no message recieved = 0, error = -1

// Function declarations
int pcanTx(int id, int data);
int pcanRx(int num_msgs);


#endif
