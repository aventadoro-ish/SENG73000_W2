#include <Arduino.h>

#include "pin_definition.h"
#include "Motor.h"
#include "CAN.h"
#include "utils.h"



// #define HOME_ON_STARTUP
#define ENERGIZE_ON_STARTUP
// #define USE_MOTOR_SERIAL
#define USE_SIMPLIFIED_CAN_PROTOCOL



void process_CAN_msg_simplified_mode(CAN_message_t rxMsg);
void process_CAN_msg_full_mode(CAN_message_t rxMsg);


// -----------------------------------------------------------------------------
// Arduino setup and loop
// -----------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ;
    }
    Serial.println("LiF EC - Setup Started");

    LiF_CAN::setup();
    // int floor_counter = 0;
    // for (;;) {
    //     LiF_CAN::transmit(floor_counter++ % 3);
    //     delay(1);
    // }

    
    LiF_Motor::setup();
    LiF_Motor::setupMotorControlTimer();

#ifdef ENERGIZE_ON_STARTUP
    LiF_Motor::energizeCoils();
#endif

#ifdef HOME_ON_STARTUP
    // Non-blocking: tickISR() performs the sequence in the background.
    if (!LiF_Motor::startHoming()) {
        Serial.print("Unable to start homing: ");
        Serial.println(
            LiF_Motor::homingErrorName(LiF_Motor::getHomingError()));
    }
#endif


}

void loop() {
#ifdef USE_MOTOR_SERIAL
    LiF_Motor_Test::motorSerialTest();
#endif
    CAN_message_t rxMsg;

    while (LiF_CAN::bus.read(rxMsg)) {
        // CAN Message received
#ifdef USE_SIMPLIFIED_CAN_PROTOCOL
        process_CAN_msg_simplified_mode(rxMsg);
#else
        process_CAN_msg_full_mode(rxMsg);
#endif

    }

}


void process_CAN_msg_simplified_mode(CAN_message_t rxMsg) {
    Serial.printf(
        "Received CAN msg (S): ID:%x, DATA: %x %x %x %x %x %x %x %x\r\n",
        rxMsg.id,
        rxMsg.buf[0],
        rxMsg.buf[1],
        rxMsg.buf[2],
        rxMsg.buf[3],
        rxMsg.buf[4],
        rxMsg.buf[5],
        rxMsg.buf[6],
        rxMsg.buf[7]
    );

    if (rxMsg.buf[0] == 1) {
        DEBUG_PRINTLN("Moving to floor 1");
        LiF_Motor::moveToSteps(500);
    } else if (rxMsg.buf[0] == 2) {
        DEBUG_PRINTLN("Moving to floor 2");
        LiF_Motor::moveToSteps(1000);

    } else if (rxMsg.buf[0] == 3) {
        DEBUG_PRINTLN("Moving to floor 3");
        LiF_Motor::moveToSteps(1500);
        
    } else {
        DEBUG_PRINTLN("Unknown message format");
    }

}

void process_CAN_msg_full_mode(CAN_message_t rxMsg) {

}