#include <Arduino.h>

#include "pin_definition.h"
#include "Motor.h"
#include "CAN.h"




// Non-blocking Serial command parser implemented near the bottom of this file.
// Call it continuously from loop().
void motorSerialTest();


// Choose a timer not used by another library/peripheral in this project.
HardwareTimer motorControlTimer(TIM6);

namespace {

constexpr size_t SERIAL_COMMAND_LENGTH = 96;
char serialCommand[SERIAL_COMMAND_LENGTH];
size_t serialCommandLength = 0;
bool statusWatchEnabled = false;
uint32_t previousWatchPrintMs = 0;

void motorControlTimerISR() {
    LiF_Motor::tickISR();
}

void setupMotorControlTimer() {
    motorControlTimer.pause();
    motorControlTimer.setOverflow(
        LiF_Motor::CONTROL_TICK_HZ,
        HERTZ_FORMAT);
    motorControlTimer.attachInterrupt(motorControlTimerISR);
    motorControlTimer.refresh();
    motorControlTimer.resume();
}

bool parseFloat(const char *text, float &value) {
    if (text == nullptr || *text == '\0') {
        return false;
    }

    char *end = nullptr;
    const float parsed = strtof(text, &end);
    if (end == text || *end != '\0') {
        return false;
    }

    value = parsed;
    return true;
}

bool parseInt32(const char *text, int32_t &value) {
    if (text == nullptr || *text == '\0') {
        return false;
    }

    char *end = nullptr;
    const long parsed = strtol(text, &end, 10);
    if (end == text || *end != '\0') {
        return false;
    }

    value = static_cast<int32_t>(parsed);
    return true;
}

void printResult(bool accepted) {
    Serial.println(accepted ? F("OK") : F("Rejected"));
}

void printMotorStatus() {
    Serial.println(F("--- Motor status ---"));

    Serial.print(F("step mode: "));
    Serial.println(
        LiF_Motor::stepModeName(LiF_Motor::getStepMode()));

    Serial.print(F("motion: "));
    Serial.println(
        LiF_Motor::motionModeName(LiF_Motor::getMotionMode()));

    Serial.print(F("coils: "));
    Serial.println(
        LiF_Motor::coilsAreEnergized() ? F("energized") : F("released"));

    Serial.print(F("position: "));
    Serial.print(LiF_Motor::getPositionStepsExact(), 1);
    Serial.print(F(" full steps; "));
    Serial.print(LiF_Motor::getPositionHalfSteps());
    Serial.print(F(" half steps; "));
    Serial.print(LiF_Motor::getPositionMm(), 3);
    Serial.println(F(" mm"));

    Serial.print(F("target: "));
    Serial.print(LiF_Motor::getTargetPositionHalfSteps());
    Serial.print(F(" half steps; "));
    Serial.print(LiF_Motor::getTargetPositionMm(), 3);
    Serial.println(F(" mm"));

    Serial.print(F("speed: current="));
    Serial.print(LiF_Motor::getCurrentSpeedSteps(), 2);
    Serial.print(F(", requested="));
    Serial.print(LiF_Motor::getTargetSpeedSteps(), 2);
    Serial.println(F(" full steps/s"));

    Serial.print(F("limits: lower="));
    Serial.print(LiF_Motor::lowerLimitIsActive() ? F("ACTIVE") : F("open"));
    Serial.print(F(", upper="));
    Serial.println(LiF_Motor::upperLimitIsActive() ? F("ACTIVE") : F("open"));

    Serial.print(F("homing: "));
    Serial.print(
        LiF_Motor::homingStateName(LiF_Motor::getHomingState()));
    Serial.print(F(", error="));
    Serial.println(
        LiF_Motor::homingErrorName(LiF_Motor::getHomingError()));

    Serial.print(F("range: "));
    if (LiF_Motor::isHomed()) {
        Serial.print(LiF_Motor::getTravelRangeHalfSteps());
        Serial.print(F(" half steps; "));
        Serial.print(LiF_Motor::getTravelRangeMm(), 3);
        Serial.println(F(" mm"));
    } else {
        Serial.println(F("not calibrated"));
    }
}

void printMotorTestHelp() {
    Serial.println(F("\nLiF motor test commands:"));
    Serial.println(F("  help                         show this list"));
    Serial.println(F("  status                       print complete state"));
    Serial.println(F("  watch on|off                 periodic status output"));
    Serial.println(F("  energize                     energize/hold coils"));
    Serial.println(F("  release                      stop and release coils"));
    Serial.println(F("  mode full|half               set mode while released"));
    Serial.println(F("  speed <full-steps/s>          continuous signed speed"));
    Serial.println(F("  speedmm <mm/s>               continuous signed speed"));
    Serial.println(F("  move <full-steps> [speed]     absolute position"));
    Serial.println(F("  movehalf <half-steps> [speed] absolute half-step position"));
    Serial.println(F("  movemm <mm> [mm/s]           absolute mm position"));
    Serial.println(F("  zero [full-steps]             assign current coordinate"));
    Serial.println(F("  zerohalf [half-steps]         assign half-step coordinate"));
    Serial.println(F("  home                         run upper/lower homing"));
    Serial.println(F("  cancelhome                   cancel homing"));
    Serial.println(F("  stop                         acceleration-limited stop"));
    Serial.println(F("  estop                        immediate stop"));
    Serial.println();
}

void executeMotorCommand(char *line) {
    char *save = nullptr;
    char *command = strtok_r(line, " \t", &save);
    if (command == nullptr) {
        return;
    }

    if (strcmp(command, "help") == 0 || strcmp(command, "?") == 0) {
        printMotorTestHelp();
        return;
    }

    if (strcmp(command, "status") == 0) {
        printMotorStatus();
        return;
    }

    if (strcmp(command, "watch") == 0) {
        const char *argument = strtok_r(nullptr, " \t", &save);
        if (argument != nullptr && strcmp(argument, "on") == 0) {
            statusWatchEnabled = true;
            Serial.println(F("Status watch enabled"));
        } else if (argument != nullptr && strcmp(argument, "off") == 0) {
            statusWatchEnabled = false;
            Serial.println(F("Status watch disabled"));
        } else {
            Serial.println(F("Usage: watch on|off"));
        }
        return;
    }

    if (strcmp(command, "energize") == 0 || strcmp(command, "on") == 0) {
        LiF_Motor::energizeCoils();
        Serial.println(F("Coils energized"));
        return;
    }

    if (strcmp(command, "release") == 0 || strcmp(command, "off") == 0) {
        LiF_Motor::releaseCoils();
        Serial.println(F("Coils released"));
        return;
    }

    if (strcmp(command, "mode") == 0) {
        const char *argument = strtok_r(nullptr, " \t", &save);
        if (argument != nullptr && strcmp(argument, "full") == 0) {
            printResult(LiF_Motor::setStepMode(
                LiF_Motor::StepMode::FULL_STEP));
        } else if (argument != nullptr && strcmp(argument, "half") == 0) {
            printResult(LiF_Motor::setStepMode(
                LiF_Motor::StepMode::HALF_STEP));
        } else {
            Serial.println(F("Usage: mode full|half"));
        }
        return;
    }

    if (strcmp(command, "speed") == 0 || strcmp(command, "speedmm") == 0) {
        float speed = 0.0f;
        if (!parseFloat(strtok_r(nullptr, " \t", &save), speed)) {
            Serial.println(F("Usage: speed <full-steps/s> or speedmm <mm/s>"));
            return;
        }

        const bool accepted = (strcmp(command, "speed") == 0)
                                  ? LiF_Motor::setTargetSpeedSteps(speed)
                                  : LiF_Motor::setTargetSpeedMm(speed);
        printResult(accepted);
        return;
    }

    if (strcmp(command, "move") == 0 ||
        strcmp(command, "movehalf") == 0) {
        int32_t target = 0;
        if (!parseInt32(strtok_r(nullptr, " \t", &save), target)) {
            Serial.println(F("Usage: move <steps> [speed] or movehalf <half-steps> [speed]"));
            return;
        }

        float speed = LiF_Motor::DEFAULT_MOVE_SPEED_STEPS_S;
        const char *speedText = strtok_r(nullptr, " \t", &save);
        if (speedText != nullptr && !parseFloat(speedText, speed)) {
            Serial.println(F("Invalid speed"));
            return;
        }

        const bool accepted = (strcmp(command, "move") == 0)
                                  ? LiF_Motor::moveToSteps(target, speed)
                                  : LiF_Motor::moveToHalfSteps(target, speed);
        printResult(accepted);
        return;
    }

    if (strcmp(command, "movemm") == 0) {
        float targetMm = 0.0f;
        if (!parseFloat(strtok_r(nullptr, " \t", &save), targetMm)) {
            Serial.println(F("Usage: movemm <mm> [mm/s]"));
            return;
        }

        float speedMm =
            LiF_Motor::DEFAULT_MOVE_SPEED_STEPS_S * LiF_Motor::MM_PER_STEP;
        const char *speedText = strtok_r(nullptr, " \t", &save);
        if (speedText != nullptr && !parseFloat(speedText, speedMm)) {
            Serial.println(F("Invalid speed"));
            return;
        }

        printResult(LiF_Motor::moveToMm(targetMm, speedMm));
        return;
    }

    if (strcmp(command, "zero") == 0 ||
        strcmp(command, "zerohalf") == 0) {
        int32_t coordinate = 0;
        const char *coordinateText = strtok_r(nullptr, " \t", &save);
        if (coordinateText != nullptr &&
            !parseInt32(coordinateText, coordinate)) {
            Serial.println(F("Invalid coordinate"));
            return;
        }

        const bool accepted = (strcmp(command, "zero") == 0)
                                  ? LiF_Motor::setCurrentPositionSteps(coordinate)
                                  : LiF_Motor::setCurrentPositionHalfSteps(coordinate);
        printResult(accepted);
        return;
    }

    if (strcmp(command, "home") == 0) {
        printResult(LiF_Motor::startHoming());
        return;
    }

    if (strcmp(command, "cancelhome") == 0) {
        LiF_Motor::cancelHoming();
        Serial.println(F("Homing cancelled"));
        return;
    }

    if (strcmp(command, "stop") == 0) {
        LiF_Motor::stop();
        Serial.println(F("Controlled stop requested"));
        return;
    }

    if (strcmp(command, "estop") == 0) {
        LiF_Motor::emergencyStop();
        Serial.println(F("Emergency stop applied"));
        return;
    }

    Serial.println(F("Unknown command. Enter 'help'."));
}

} // namespace

// Call this repeatedly from loop(). It never blocks waiting for input.
void motorSerialTest() {
    while (Serial.available() > 0) {
        const char received = static_cast<char>(Serial.read());

        if (received == '\r' || received == '\n') {
            if (serialCommandLength > 0) {
                serialCommand[serialCommandLength] = '\0';
                executeMotorCommand(serialCommand);
                serialCommandLength = 0;
                Serial.print(F("> "));
            }
            continue;
        }

        if ((received == '\b' || received == 127) &&
            serialCommandLength > 0) {
            --serialCommandLength;
            continue;
        }

        if (serialCommandLength < SERIAL_COMMAND_LENGTH - 1) {
            serialCommand[serialCommandLength++] = received;
        } else {
            serialCommandLength = 0;
            Serial.println(F("\nCommand too long; buffer cleared"));
            Serial.print(F("> "));
        }
    }

    if (statusWatchEnabled &&
        millis() - previousWatchPrintMs >= 500U) {
        previousWatchPrintMs = millis();
        printMotorStatus();
    }
}

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
    setupMotorControlTimer();

    LiF_Motor::energizeCoils();

    // Non-blocking: tickISR() performs the sequence in the background.
    if (!LiF_Motor::startHoming()) {
        Serial.print("Unable to start homing: ");
        Serial.println(
            LiF_Motor::homingErrorName(LiF_Motor::getHomingError()));
    }


}

void loop() {
    motorSerialTest();
    
    // static uint32_t previousPrintMs = 0;
    // static bool commandedMidpoint = false;

    // // Slow I2C work belongs here, never in motorControlTimerISR().
    // // Example for a future sensor:
    // // float tofPositionMm = readTofSensor();
    // // LiF_Motor::submitTofMeasurementMm(tofPositionMm, millis());

    // if (LiF_Motor::isHomed() && !commandedMidpoint) {
    //     const int32_t midpoint = LiF_Motor::getTravelRangeSteps() / 2;
    //     commandedMidpoint = LiF_Motor::moveToSteps(midpoint, 100.0f);
    // }

    // if (millis() - previousPrintMs >= 250U) {
    //     previousPrintMs = millis();

    //     Serial.print("mode=");
    //     Serial.print(
    //         LiF_Motor::motionModeName(LiF_Motor::getMotionMode()));
    //     Serial.print(" homing=");
    //     Serial.print(
    //         LiF_Motor::homingStateName(LiF_Motor::getHomingState()));
    //     Serial.print(" position=");
    //     Serial.print(LiF_Motor::getPositionSteps());
    //     Serial.print(" steps, speed=");
    //     Serial.print(LiF_Motor::getCurrentSpeedSteps());
    //     Serial.print(" steps/s, lower=");
    //     Serial.print(LiF_Motor::lowerLimitIsActive());
    //     Serial.print(" upper=");
    //     Serial.println(LiF_Motor::upperLimitIsActive());
    // }
}