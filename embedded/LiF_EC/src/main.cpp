#include <Arduino.h>
#include <Stepper.h>

#if defined(STM32_ENV)
#include "STM32_CAN.h"

#else
#error "Please, define an embedded platform to be used.Either ESP32_ENV or STM32_ENV flags"
#endif


constexpr int STEPS_PER_REVOLUTION = 360 / 1.8;  // 200 steps/revolution

#if defined(ESP32_ENV)
constexpr int IN1 = 23;
constexpr int IN2 = 19;
constexpr int IN3 = 22;
constexpr int IN4 = 18;

#elif defined(STM32_ENV)
constexpr int IN1 = PC1; // A1
constexpr int IN2 = PC0; // A2
constexpr int IN3 = PC2; // B1
constexpr int IN4 = PC3; // B2

constexpr int CAN_Tx = PB9;
constexpr int CAN_Rx = PB8;
constexpr int CAN_stb = PC8;


#else
#error "Please, define an embedded platform to be used.Either ESP32_ENV or STM32_ENV flags"
#endif



// CAN message parameters and other definitions
constexpr int TxID = 0x101;
constexpr int DLC = 1;
constexpr int Floor1 = 0x05;
constexpr int Floor2 = 0x06;
constexpr int Floor3 = 0x07;

// Sets the care/don't care bits in the ID (11 bits long for Standard CAN) [first 4 nibbles] and first two bytes of data [last four nibbles]. Using this mask we care about all ID bits so that ID of a message must match a filter below.    
constexpr int MASK = 0x07FF0000;                    // Mask0 == MASK1 == MASK --> Issue with Hardware used - MASK0 and MASK1 must both be set and be the same in order to filter properly.
constexpr int FILTER_SC = 0x01000000;                // Acceptance filter for ID 0x100 (Supervisory Controller - Raspberry Pi)
constexpr int FILTER_EC = 0x01010000;                // Acceptance filter for ID 0x101 (Elevator Controller) -- comment out if only want to accept commands from Supervisory controller
constexpr int FILTER_CC = 0x02000000;                // Acceptance filter for ID 0x200 (Car Controller)      -- comment out if only want to accept commands from Supervisory controller
constexpr int FILTER_F1 = 0x02010000;                // Acceptance filter for ID 0x201 (Floor 1 Controller)  -- comment out if only want to accept commands from Supervisory controller
constexpr int FILTER_F2 = 0x02020000;                // Acceptance filter for ID 0x202 (Floor 2 Controller)  -- comment out if only want to accept commands from Supervisory controller
constexpr int FILTER_F3 = 0x02030000;                // Acceptance filter for ID 0x203 (Floor 3 Controller)  -- comment out if only want to accept commands from Supervisory controller



constexpr long SWEEP_MIN_STEP = 0;
constexpr long SWEEP_MAX_STEP = 1000;

Stepper myStepper(STEPS_PER_REVOLUTION, IN1, IN2, IN3, IN4);

STM32_CAN CAN_bus(CAN_Rx, CAN_Tx);



enum class Direction : int8_t {
    CLOCKWISE = 1,
    COUNTERCLOCKWISE = -1
};

enum class MotionMode {
    STOPPED,
    MANUAL,
    SWEEP
};

long selectedRPM = 10;
long currentStep = 0;
long sweepTarget = SWEEP_MAX_STEP;

Direction direction = Direction::CLOCKWISE;
MotionMode motionMode = MotionMode::STOPPED;

String serialLine;


// -----------------------------------------------------------------------------
// Utility functions
// -----------------------------------------------------------------------------

const char *directionName()
{
    return direction == Direction::CLOCKWISE
               ? "clockwise"
               : "counterclockwise";
}

const char *motionModeName()
{
    switch (motionMode) {
        case MotionMode::STOPPED:
            return "stopped";

        case MotionMode::MANUAL:
            return "manual continuous";

        case MotionMode::SWEEP:
            return "0-1000 sweep";

        default:
            return "unknown";
    }
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

void releaseMotor()
{
    motionMode = MotionMode::STOPPED;

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

    myStepper.step(stepDirection);
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

    myStepper.step(stepDirection);
    currentStep += stepDirection;
}




// -----------------------------------------------------------------------------
// CAN Bus setup and helper functions
// -----------------------------------------------------------------------------
void setup_CAN_bus() {
    // CAN_bus.setIRQPriority(uint32_t preemptPriority, uint32_t subPriority); // default: lowest prio, 0
    // CAN_bus.setAutoRetransmission(bool enabled);  //default: true
    // CAN_bus.setRxFIFOLock(bool fifo0locked);      //default: false
    // CAN_bus.setTxBufferMode(TX_BUFFER_MODE mode); //default: FIFO
    // CAN_bus.setTimestampCounter(false);    // should be set false as per lib documentation
    // CAN_bus.setMode(MODE mode);                   //default: NORMAL
    // CAN_bus.setAutoBusOffRecovery(bool enabled);  //default: false
    pinMode(CAN_stb, OUTPUT);
    digitalWrite(CAN_stb, LOW);
    CAN_bus.setBaudRate(125000);

    // Exact-match standard ID filters
    CAN_bus.setFilterSingleMask(0, 0x100, 0x7FF, STD); // Supervisory Controller
    CAN_bus.setFilterSingleMask(1, 0x101, 0x7FF, STD); // Elevator Controller
    CAN_bus.setFilterSingleMask(2, 0x200, 0x7FF, STD); // Car Controller
    CAN_bus.setFilterSingleMask(3, 0x201, 0x7FF, STD); // Floor 1
    CAN_bus.setFilterSingleMask(4, 0x202, 0x7FF, STD); // Floor 2
    CAN_bus.setFilterSingleMask(5, 0x203, 0x7FF, STD); // Floor 3

    CAN_bus.begin();
}


bool transmitCAN(uint8_t floorByte) {
    CAN_message_t msg;

    msg.id = TxID;
    msg.flags.extended = false;
    msg.flags.remote = false;
    msg.len = DLC;
    msg.buf[0] = floorByte;

    return CAN_bus.write(msg);
}

bool receiveCAN(uint32_t &rxId, uint8_t &rxLen, uint8_t rxData[8]) {
    CAN_message_t msg;

    if (!CAN_bus.read(msg)) {
        return false;
    }

    rxId = msg.id;
    rxLen = msg.len;

    for (uint8_t i = 0; i < msg.len && i < 8; i++) {
        rxData[i] = msg.buf[i];
    }

    return true;
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

    setup_CAN_bus();

    int floor_counter = 0;
    for (;;) {
        transmitCAN(floor_counter++ % 3);
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