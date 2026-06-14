#include <Arduino.h>

#include "pin_definition.h"
#include "Motor.h"
#include "CAN.h"


// -----------------------------------------------------------------------------
// Arduino setup and loop
// -----------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);



    Serial.println();
    Serial.println("ESP32 stepper debug interface");
    Serial.println("Enter 'help' for available commands.");

    LiF_CAN::setup();
    LiF_Motor::setup();

    int floor_counter = 0;
    for (;;) {
        LiF_CAN::transmit(floor_counter++ % 3);
        delay(1);
    }

}

void loop() {

}