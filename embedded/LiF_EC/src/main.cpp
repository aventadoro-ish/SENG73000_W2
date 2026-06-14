#include <Arduino.h>
#include <Stepper.h>

const int stepsPerRevolution = 360 / 1.8;  // change this to fit the number of steps per revolution

// ULN2003 Motor Driver Pins
// #define IN1 23
// #define IN2 22
// #define IN3 19
// #define IN4 18

// #define IN1 22
// #define IN2 23
// #define IN3 19
// #define IN4 18

// #define IN1 22
// #define IN2 23
// #define IN3 18
// #define IN4 19

// Works
#define IN1 23
#define IN2 19
#define IN3 22
#define IN4 18






// initialize the stepper library
Stepper myStepper(stepsPerRevolution, IN1, IN3, IN2, IN4);



void setup() {
    // set the speed at 5 rpm
    myStepper.setSpeed(10);
    // initialize the serial port
    Serial.begin(115200);
}

void loop() {
    // step one revolution in one direction:
    Serial.println("clockwise");
    myStepper.step(stepsPerRevolution);
    delay(1000);

    // step one revolution in the other direction:
    Serial.println("counterclockwise");
    myStepper.step(-stepsPerRevolution);
    delay(1000);

}
