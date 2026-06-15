#include <Arduino.h>

#include "pin_definition.h"
#include "Motor.h"
#include "CAN.h"








// -----------------------------------------------------------------------------
// Arduino setup and loop
// -----------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);

    LiF_CAN::setup();
    // int floor_counter = 0;
    // for (;;) {
    //     LiF_CAN::transmit(floor_counter++ % 3);
    //     delay(1);
    // }

    
    LiF_Motor::setup();
    LiF_Motor::setupMotorControlTimer();

    LiF_Motor::energizeCoils();

    // Non-blocking: tickISR() performs the sequence in the background.
    if (!LiF_Motor::startHoming()) {
        Serial.print("Unable to start homing: ");
        Serial.println(
            LiF_Motor::homingErrorName(LiF_Motor::getHomingError()));
    }


}

void loop() {
    LiF_Motor_Test::motorSerialTest();
    
}