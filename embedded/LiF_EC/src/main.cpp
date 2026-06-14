#include <Arduino.h>



#include "pin_definition.h"
#include "Motor.h"
#include "CAN.h"
















String serialLine;


// -----------------------------------------------------------------------------
// Utility functions
// -----------------------------------------------------------------------------

void printPosition()
{
    Serial.print("Current commanded step: ");
    Serial.println(currentStep);
}

void printStatus()
{
    Serial.println();
    Serial.println("--- Motor status ---");

    Serial.print("Mode:       ");
    Serial.println(motionModeName());

    Serial.print("Direction:  ");
    Serial.println(directionName());

    Serial.print("Speed:      ");
    Serial.print(selectedRPM);
    Serial.println(" RPM");

    Serial.print("Position:   ");
    Serial.print(currentStep);
    Serial.println(" steps");

    if (motionMode == MotionMode::SWEEP) {
        Serial.print("Target:     ");
        Serial.print(sweepTarget);
        Serial.println(" steps");
    }

    Serial.println("--------------------");
}

void printHelp()
{
    Serial.println();
    Serial.println("Available commands:");
    Serial.println("  speed <rpm>        Set motor speed");
    Serial.println("  start              Start in selected direction");
    Serial.println("  start cw           Start clockwise");
    Serial.println("  start ccw          Start counterclockwise");
    Serial.println("  dir cw             Select clockwise direction");
    Serial.println("  dir ccw            Select counterclockwise direction");
    Serial.println("  sweep              Cycle between steps 0 and 1000");
    Serial.println("  stop               Stop motor; retain holding torque");
    Serial.println("  release            Stop and de-energize motor coils");
    Serial.println("  zero               Set current step counter to zero");
    Serial.println("  pos                 Print current step counter");
    Serial.println("  status              Print all motor settings");
    Serial.println("  help                Print this command list");
    Serial.println();
}



// -----------------------------------------------------------------------------
// Command handling
// -----------------------------------------------------------------------------

bool setDirectionFromText(const String &argument)
{
    if (argument == "cw" || argument == "clockwise") {
        direction = Direction::UP;
        return true;
    }

    if (argument == "ccw" ||
        argument == "counterclockwise" ||
        argument == "anticlockwise") {
        direction = Direction::DOWN;
        return true;
    }

    return false;
}

void handleCommand(String line)
{
    line.trim();
    line.toLowerCase();

    if (line.length() == 0) {
        return;
    }

    int separator = line.indexOf(' ');

    String command;
    String argument;

    if (separator < 0) {
        command = line;
    } else {
        command = line.substring(0, separator);
        argument = line.substring(separator + 1);
        argument.trim();
    }

    if (command == "help" || command == "?") {
        printHelp();
    }

    else if (command == "speed" || command == "rpm") {
        long newRPM = argument.toInt();

        if (argument.length() == 0 || newRPM <= 0) {
            Serial.println("Error: speed must be a positive integer.");
            Serial.println("Example: speed 15");
            return;
        }

        selectedRPM = newRPM;
        motor.setSpeed(selectedRPM);

        Serial.print("Speed set to ");
        Serial.print(selectedRPM);
        Serial.println(" RPM.");
    }

    else if (command == "start") {
        if (argument.length() > 0 && !setDirectionFromText(argument)) {
            Serial.println("Error: direction must be cw or ccw.");
            return;
        }

        motionMode = MotionMode::MANUAL;

        Serial.print("Motor started ");
        Serial.print(directionName());
        Serial.print(" at ");
        Serial.print(selectedRPM);
        Serial.println(" RPM.");
    }

    else if (command == "sweep" || command == "cycle") {
        motionMode = MotionMode::SWEEP;

        /*
         * If the motor is not currently at logical position zero,
         * return to zero before beginning the 0-to-1000 cycle.
         */
        if (currentStep != SWEEP_MIN_STEP) {
            sweepTarget = SWEEP_MIN_STEP;
            Serial.println(
                "Sweep enabled. Returning to step 0 before cycling."
            );
        } else {
            sweepTarget = SWEEP_MAX_STEP;
            Serial.println(
                "Sweep enabled: cycling between steps 0 and 1000."
            );
        }
    }

    else if (command == "stop") {
        motionMode = MotionMode::STOPPED;
        Serial.println("Motor stopped. Coils remain energized.");
    }

    else if (command == "release") {
        releaseMotor();
    }

    else if (command == "dir") {
        if (!setDirectionFromText(argument)) {
            Serial.println("Error: use 'dir cw' or 'dir ccw'.");
            return;
        }

        Serial.print("Direction set to ");
        Serial.print(directionName());
        Serial.println(".");
    }

    else if (command == "cw") {
        direction = Direction::UP;
        Serial.println("Direction set to clockwise.");
    }

    else if (command == "ccw") {
        direction = Direction::DOWN;
        Serial.println("Direction set to counterclockwise.");
    }

    else if (command == "zero") {
        currentStep = 0;
        Serial.println("Step counter set to zero.");

        if (motionMode == MotionMode::SWEEP) {
            sweepTarget = SWEEP_MAX_STEP;
            Serial.println("Sweep target reset to 1000.");
        }
    }

    else if (command == "pos" || command == "position") {
        printPosition();
    }

    else if (command == "status") {
        printStatus();
    }

    else {
        Serial.print("Unknown command: ");
        Serial.println(command);
        Serial.println("Enter 'help' for a list of commands.");
    }
}

void readSerialCommands()
{
    while (Serial.available() > 0) {
        char received = static_cast<char>(Serial.read());

        if (received == '\r') {
            continue;
        }

        if (received == '\n') {
            handleCommand(serialLine);
            serialLine = "";
        }
        else if (serialLine.length() < 80) {
            serialLine += received;
        }
        else {
            serialLine = "";
            Serial.println("Error: command was too long.");
        }
    }
}


// -----------------------------------------------------------------------------
// Motor operation
// -----------------------------------------------------------------------------

void runManualMode()
{
    int stepDirection = static_cast<int>(direction);

    motor.step(stepDirection);
    currentStep += stepDirection;
}

void runSweepMode()
{
    /*
     * Change direction whenever the current target is reached.
     */
    if (currentStep == sweepTarget) {
        if (sweepTarget == SWEEP_MAX_STEP) {
            sweepTarget = SWEEP_MIN_STEP;
        } else {
            sweepTarget = SWEEP_MAX_STEP;
        }
    }

    int stepDirection;

    if (currentStep < sweepTarget) {
        stepDirection = 1;
    } else {
        stepDirection = -1;
    }

    motor.step(stepDirection);
    currentStep += stepDirection;
}


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

    printStatus();
}

void loop()
{
    readSerialCommands();

    switch (motionMode) {
        case MotionMode::MANUAL:
            runManualMode();
            break;

        case MotionMode::SWEEP:
            runSweepMode();
            break;

        case MotionMode::STOPPED:
        default:
            break;
    }
}