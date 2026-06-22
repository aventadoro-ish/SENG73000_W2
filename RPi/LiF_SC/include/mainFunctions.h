#ifndef MAIN_FUNCTIONS
#define MAIN_FUNCTIONS

// Queue 
#define QUEUE_SIZE 3

extern int requestQueue[QUEUE_SIZE];
extern int queueHead;
extern int queueTail;
extern int queueCount;

void enqueueFloor(int floor);
int dequeueFloor();

// Main menu
int menu();
int chooseID();
int chooseMsg();
int HexFromFloor(int floorVal);
int FloorFromHex(int Hex);

// States
typedef enum {
    STATE_IDLE,        // 0 - no message request, check DB and CAN for requests
    STATE_MOVING,      // 1 - message to EC, cart is moving and waiting for ARRIVED target floor
    STATE_ARRIVED,     // 2 - EC reached it's target floor, updating DB, setting to IDLE
    STATE_FAULT        // 3 - Before timeout exists
} State;

extern State currentState;
extern int ccDoorClosed; //0 - door open, 1 - door closed

#define MOVE_TIMEOUT_SEC 15

#endif
