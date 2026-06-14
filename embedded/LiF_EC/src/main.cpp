#include <Arduino.h>
#include <Stepper.h>

constexpr int STEPS_PER_REVOLUTION = 360 / 1.8;  // 200 full steps/revolution

#define IN1 23
#define IN2 19
#define IN3 22
#define IN4 18

// Preserve the phase order from your original program.
Stepper myStepper(
    STEPS_PER_REVOLUTION,
    IN1,
    IN3,
    IN2,
    IN4
);

enum class Direction : int8_t {
    CLOCKWISE = 1,
    COUNTERCLOCKWISE = -1
};

long selectedRPM = 10;
long currentStep = 0;

Direction direction = Direction::CLOCKWISE;
bool motorRunning = false;

String serialLine;


// -----------------------------------------------------------------------------
// Utility functions
// -----------------------------------------------------------------------------

const char *directionName()
{
    return direction == Direction::CLOCKWISE ? "clockwise" : "counterclockwise";
}

void printPosition()
{
    Serial.print("Current commanded step: ");
    Serial.println(currentStep);
}

void printStatus()
{
    Serial.println();
    Serial.println("--- Motor status ---");

    Serial.print("State:     ");
    Serial.println(motorRunning ? "running" : "stopped");

    Serial.print("Direction: ");
    Serial.println(directionName());

    Serial.print("Speed:     ");
    Serial.print(selectedRPM);
    Serial.println(" RPM");

    Serial.print("Position:  ");
    Serial.print(currentStep);
    Serial.println(" steps");

    Serial.println("--------------------");
}

void printHelp()
{
    Serial.println();
    Serial.println("Available commands:");
    Serial.println("  speed <rpm>        Set rotation speed");
    Serial.println("  start              Start using current direction");
    Serial.println("  start cw           Start clockwise");
    Serial.println("  start ccw          Start counterclockwise");
    Serial.println("  dir cw             Select clockwise direction");
    Serial.println("  dir ccw            Select counterclockwise direction");
    Serial.println("  stop               Stop stepping; retain holding torque");
    Serial.println("  release            Stop and de-energize motor coils");
    Serial.println("  zero               Set current step counter to zero");
    Serial.println("  pos                 Print current step counter");
    Serial.println("  status              Print all motor settings");
    Serial.println("  help                Print this command list");
    Serial.println();
}

void releaseMotor()
{
    motorRunning = false;

    // Remove the drive signal from all phases.
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);

    Serial.println("Motor stopped and outputs released.");
}


// -----------------------------------------------------------------------------
// Command handling
// -----------------------------------------------------------------------------

bool setDirectionFromText(const String &argument)
{
    if (argument == "cw" || argument == "clockwise") {
        direction = Direction::CLOCKWISE;
        return true;
    }

    if (argument == "ccw" ||
        argument == "counterclockwise" ||
        argument == "anticlockwise") {
        direction = Direction::COUNTERCLOCKWISE;
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

    // Separate the command from its argument.
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
        myStepper.setSpeed(selectedRPM);

        Serial.print("Speed set to ");
        Serial.print(selectedRPM);
        Serial.println(" RPM.");
    }

    else if (command == "start") {
        if (argument.length() > 0 && !setDirectionFromText(argument)) {
            Serial.println("Error: direction must be cw or ccw.");
            return;
        }

        motorRunning = true;

        Serial.print("Motor started ");
        Serial.print(directionName());
        Serial.print(" at ");
        Serial.print(selectedRPM);
        Serial.println(" RPM.");
    }

    else if (command == "stop") {
        motorRunning = false;
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
        direction = Direction::CLOCKWISE;
        Serial.println("Direction set to clockwise.");
    }

    else if (command == "ccw") {
        direction = Direction::COUNTERCLOCKWISE;
        Serial.println("Direction set to counterclockwise.");
    }

    else if (command == "zero") {
        currentStep = 0;
        Serial.println("Step counter set to zero.");
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
        } else if (serialLine.length() < 80) {
            serialLine += received;
        } else {
            serialLine = "";
            Serial.println("Error: command was too long.");
        }
    }
}


// -----------------------------------------------------------------------------
// Arduino setup and loop
// -----------------------------------------------------------------------------

void setup()
{
    Serial.begin(115200);

    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);

    myStepper.setSpeed(selectedRPM);

    Serial.println();
    Serial.println("ESP32 stepper debug interface");
    Serial.println("Enter 'help' for available commands.");
    printStatus();
}

void loop()
{
    readSerialCommands();

    if (motorRunning) {
        int stepDirection = static_cast<int>(direction);

        // Stepper.step() is blocking, but only for one step here.
        myStepper.step(stepDirection);

        // This is commanded position, not encoder-measured position.
        currentStep += stepDirection;
    }
}