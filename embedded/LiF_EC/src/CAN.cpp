#include "CAN.h"
#include "pin_definition.h"


namespace LiF_CAN {
    
STM32_CAN bus(CAN_Rx, CAN_Tx);


void setup() {
    // bus.setIRQPriority(uint32_t preemptPriority, uint32_t subPriority); // default: lowest prio, 0
    // bus.setAutoRetransmission(bool enabled);  //default: true
    // bus.setRxFIFOLock(bool fifo0locked);      //default: false
    // bus.setTxBufferMode(TX_BUFFER_MODE mode); //default: FIFO
    // bus.setTimestampCounter(false);    // should be set false as per lib documentation
    // bus.setMode(MODE mode);                   //default: NORMAL
    // bus.setAutoBusOffRecovery(bool enabled);  //default: false
    
    pinMode(CAN_stb, OUTPUT);
    digitalWrite(CAN_stb, LOW);
    bus.setBaudRate(125000);

    // Exact-match standard ID filters
    bus.setFilterSingleMask(0, FILTER_SC, MASK, STD); // Supervisory Controller
    bus.setFilterSingleMask(1, FILTER_EC, MASK, STD); // Elevator Controller
    bus.setFilterSingleMask(2, FILTER_CC, MASK, STD); // Car Controller
    bus.setFilterSingleMask(3, FILTER_F1, MASK, STD); // Floor 1
    bus.setFilterSingleMask(4, FILTER_F2, MASK, STD); // Floor 2
    bus.setFilterSingleMask(5, FILTER_F3, MASK, STD); // Floor 3

    bus.begin();
}

bool transmit(uint8_t floorByte) {
    CAN_message_t msg;

    msg.id = TxID;
    msg.flags.extended = false;
    msg.flags.remote = false;
    msg.len = DLC;
    msg.buf[0] = floorByte;

    return bus.write(msg);
}

bool receive(uint32_t &rxId, uint8_t &rxLen, uint8_t rxData[8]) {
    CAN_message_t msg;

    if (!bus.read(msg)) {
        return false;
    }

    rxId = msg.id;
    rxLen = msg.len;

    for (uint8_t i = 0; i < msg.len && i < 8; i++) {
        rxData[i] = msg.buf[i];
    }

    return true;
}

} // namespace LiF_CAN