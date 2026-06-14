#pragma once

#include <Arduino.h>
#include "pin_definition.h"

#if !defined(STM32_ENV)
#error "LiF_Motor currently requires STM32_ENV"
#endif

namespace LiF_Motor {

// -----------------------------------------------------------------------------
// Mechanical and control configuration
// -----------------------------------------------------------------------------

constexpr int32_t STEPS_PER_REVOLUTION = 200; // 1.8-degree motor

// The tickISR() function must be called at exactly this frequency.
constexpr uint32_t CONTROL_TICK_HZ = 10000;

// Positive speed/position is upward. Change these values for the mechanism.
constexpr float MM_PER_STEP = 1.0f;               // TODO: set measured conversion
constexpr float MAX_ACCEL_STEPS_S2 = 300.0f;
constexpr float DEFAULT_MOVE_SPEED_STEPS_S = 150.0f;
constexpr float HOMING_SPEED_STEPS_S = 80.0f;
constexpr int32_t HOMING_MAX_LEG_STEPS = 100000;

// Set to -1 if the electrical sequence called "up" moves the mechanism down.
// This changes phase order without changing the public sign convention.
constexpr int8_t UP_PHASE_DIRECTION = 1;

// Limit-switch mapping. Swap these two aliases if the switches are reversed.
constexpr uint32_t LOWER_LIMIT_PIN = LimSw_1;
constexpr uint32_t UPPER_LIMIT_PIN = LimSw_2;

// Assumed circuit:
// NC switch -> Schmitt-trigger inverter -> MCU input.
// With the common pull-up arrangement, an actuated/open switch produces LOW.
// Change this to HIGH if your measured signal has the opposite polarity.
constexpr auto LIMIT_ACTIVE_LEVEL = LOW;
constexpr auto LIMIT_PIN_MODE = INPUT_PULLUP;
constexpr uint32_t LIMIT_DEBOUNCE_MS = 3;

// The DDS step generator can produce at most one full step per control tick.
constexpr float MAX_COMMAND_SPEED_STEPS_S =
    static_cast<float>(CONTROL_TICK_HZ) * 0.8f;

// -----------------------------------------------------------------------------
// Public state types
// -----------------------------------------------------------------------------

enum class MotionMode : uint8_t {
    IDLE,
    SPEED,
    POSITION,
    STOPPING,
    HOMING
};

enum class HomingState : uint8_t {
    NOT_HOMED,
    SEEKING_UPPER,
    SEEKING_LOWER,
    COMPLETE,
    FAILED
};

enum class HomingError : uint8_t {
    NONE,
    BOTH_LIMITS_ACTIVE,
    UPPER_NOT_FOUND,
    LOWER_NOT_FOUND,
    COILS_RELEASED
};

// -----------------------------------------------------------------------------
// Initialization and interrupt entry point
// -----------------------------------------------------------------------------

/// Configures motor outputs, limit inputs, and external limit interrupts.
/// Coils start released and the current coordinate starts at zero.
void setup();

/// Call from a hardware-timer interrupt at CONTROL_TICK_HZ.
/// Do not call Serial, Wire/I2C, delay(), or other blocking code from this ISR.
void tickISR();

// -----------------------------------------------------------------------------
// Coil and motion control
// -----------------------------------------------------------------------------

/// Applies the currently selected electrical phase and enables holding torque.
void energizeCoils();

/// Immediately stops motion and drives all four bridge inputs LOW.
void releaseCoils();

bool coilsAreEnergized();

/// Continuous-speed mode. Positive is upward; negative is downward.
/// Returns false if the command is out of range or points into an active limit.
bool setTargetSpeedSteps(float stepsPerSecond);
bool setTargetSpeedMm(float mmPerSecond);

float getTargetSpeedSteps();
float getTargetSpeedMm();
float getCurrentSpeedSteps();
float getCurrentSpeedMm();

/// Position mode. The supplied speed is a positive magnitude.
/// When homed, targets are restricted to [0, getTravelRangeSteps()].
bool moveToSteps(int32_t targetSteps,
                 float maxSpeedStepsPerSecond = DEFAULT_MOVE_SPEED_STEPS_S);
bool moveToMm(float targetMm,
              float maxSpeedMmPerSecond =
                  DEFAULT_MOVE_SPEED_STEPS_S * MM_PER_STEP);

/// Requests an acceleration-limited stop. emergencyStop() stops immediately.
void stop();
void emergencyStop();

// -----------------------------------------------------------------------------
// Position and homing
// -----------------------------------------------------------------------------

int32_t getPositionSteps();
float getPositionMm();
int32_t getTargetPositionSteps();
float getTargetPositionMm();

/// Changes only the coordinate assigned to the current physical location.
/// This is allowed only while stopped.
bool setCurrentPositionSteps(int32_t positionSteps);
bool setCurrentPositionMm(float positionMm);

/// Non-blocking homing sequence:
/// 1. Move upward until the upper switch becomes active.
/// 2. Move downward until the lower switch becomes active.
/// 3. Record the measured step range and assign the lower end position zero.
/// Progress is performed entirely by tickISR().
bool startHoming();
void cancelHoming();

bool isHomed();
bool isHoming();
HomingState getHomingState();
HomingError getHomingError();
int32_t getTravelRangeSteps();
float getTravelRangeMm();

// -----------------------------------------------------------------------------
// Limit-switch state
// -----------------------------------------------------------------------------

bool lowerLimitIsActive();
bool upperLimitIsActive();

// -----------------------------------------------------------------------------
// Future slow position feedback hook
// -----------------------------------------------------------------------------

/// Submit a ToF result from loop()/a task after completing the I2C transaction.
/// This function only stores the sample; no feedback correction is enabled yet.
void submitTofMeasurementMm(float measuredPositionMm, uint32_t sampleTimeMs);
void invalidateTofMeasurement();
bool getLatestTofMeasurementMm(float &measuredPositionMm,
                               uint32_t &sampleTimeMs);

MotionMode getMotionMode();
const char *motionModeName(MotionMode mode);
const char *homingStateName(HomingState state);
const char *homingErrorName(HomingError error);

} // namespace LiF_Motor
