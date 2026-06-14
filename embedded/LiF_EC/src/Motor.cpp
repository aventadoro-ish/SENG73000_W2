#include "Motor.h"

using namespace LiF_Motor;

/// @brief Configures stepper motor control pins and limit switch pins
void LiF_Motor::setup() {
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);

    motor.setSpeed(0);
    release();
}

void LiF_Motor::release() {
    motionMode = MotionMode::STOPPED;

    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
}

void LiF_Motor::energize() {
    motor.setSpeed(0);
}

/// @brief Moves the car up and down until limit switches are triggered
/// fe
void LiF_Motor::zero_sequence() {

}

const char* LiF_Motor::directionName() {
    return direction == Direction::UP
            ? "clockwise"
            : "counterclockwise";
}

const char* LiF_Motor::motionModeName() {
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
