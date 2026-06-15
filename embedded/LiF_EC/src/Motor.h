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

constexpr int32_t FULL_STEPS_PER_REVOLUTION = 200; // 1.8-degree motor
constexpr int32_t STEPS_PER_REVOLUTION = FULL_STEPS_PER_REVOLUTION; // legacy alias
constexpr int32_t HALF_STEPS_PER_REVOLUTION = 2 * FULL_STEPS_PER_REVOLUTION;

// The tickISR() function must be called at exactly this frequency.
constexpr uint32_t CONTROL_TICK_HZ = 10000;

// All public speed values in "steps/s" are physical full-steps per second,
// regardless of whether FULL_STEP or HALF_STEP electrical drive is selected.
// MM_PER_STEP is likewise millimetres per physical full step.
constexpr float MM_PER_STEP = 741.4f / 983.0f; // TODO: set measured conversion
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
// Change this to HIGH if the measured active level is HIGH.
constexpr auto LIMIT_ACTIVE_LEVEL = LOW;
constexpr auto LIMIT_PIN_MODE = INPUT_PULLUP;
constexpr uint32_t LIMIT_DEBOUNCE_MS = 3;

// HALF_STEP requires two electrical phase changes per physical full step.
// This conservative limit guarantees no more than one phase change per tick.
constexpr float MAX_COMMAND_SPEED_STEPS_S =
    static_cast<float>(CONTROL_TICK_HZ) * 0.4f;

// -----------------------------------------------------------------------------
// Public state types
// -----------------------------------------------------------------------------

enum class StepMode : uint8_t {
    FULL_STEP,
    HALF_STEP
};

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
/// Coils start released, FULL_STEP is selected, and position starts at zero.
void setup();

/// Call from a hardware-timer interrupt at CONTROL_TICK_HZ.
/// Do not call Serial, Wire/I2C, delay(), or blocking code from this ISR.
void tickISR();

// -----------------------------------------------------------------------------
// Electrical step mode
// -----------------------------------------------------------------------------

/// Selects two-phase full stepping or eight-state half stepping.
/// The motor must be idle and its coils must be released. Changing the mode
/// invalidates homing because the rotor may realign when re-energized.
bool setStepMode(StepMode mode);
StepMode getStepMode();

// -----------------------------------------------------------------------------
// Coil and motion control
// -----------------------------------------------------------------------------

/// Applies the current electrical phase and enables holding torque.
void energizeCoils();

/// Immediately stops motion and drives all four bridge inputs LOW.
void releaseCoils();

bool coilsAreEnergized();

/// Continuous-speed mode. Positive is upward; negative is downward.
/// Units are physical full-steps/s in either electrical step mode.
bool setTargetSpeedSteps(float fullStepsPerSecond);
bool setTargetSpeedMm(float mmPerSecond);

float getTargetSpeedSteps();
float getTargetSpeedMm();
float getCurrentSpeedSteps();
float getCurrentSpeedMm();

/// Whole-full-step position commands retained for compatibility.
bool moveToSteps(int32_t targetFullSteps,
                 float maxSpeedFullStepsPerSecond =
                     DEFAULT_MOVE_SPEED_STEPS_S);

/// Half-step-resolution position command. An odd target is rejected while
/// FULL_STEP mode is selected because it cannot land on a half-step boundary.
bool moveToHalfSteps(int32_t targetHalfSteps,
                     float maxSpeedFullStepsPerSecond =
                         DEFAULT_MOVE_SPEED_STEPS_S);

bool moveToMm(float targetMm,
              float maxSpeedMmPerSecond =
                  DEFAULT_MOVE_SPEED_STEPS_S * MM_PER_STEP);

/// Requests an acceleration-limited stop. emergencyStop() stops immediately.
void stop();
void emergencyStop();

// -----------------------------------------------------------------------------
// Position and homing
// -----------------------------------------------------------------------------

/// Whole full-step values. At an odd half-step coordinate these truncate
/// toward zero; use the half-step or mm accessors for exact position.
int32_t getPositionSteps();
int32_t getPositionHalfSteps();
float getPositionStepsExact();
float getPositionMm();

int32_t getTargetPositionSteps();
int32_t getTargetPositionHalfSteps();
float getTargetPositionMm();

/// Changes only the coordinate assigned to the current physical location.
/// This is allowed only while stopped.
bool setCurrentPositionSteps(int32_t positionFullSteps);
bool setCurrentPositionHalfSteps(int32_t positionHalfSteps);
bool setCurrentPositionMm(float positionMm);

/// Non-blocking homing sequence:
/// 1. Move upward until the upper switch becomes active.
/// 2. Move downward until the lower switch becomes active.
/// 3. Record the measured range and assign the lower end position zero.
bool startHoming();
void cancelHoming();

bool isHomed();
bool isHoming();
HomingState getHomingState();
HomingError getHomingError();

int32_t getTravelRangeSteps();
int32_t getTravelRangeHalfSteps();
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
const char *stepModeName(StepMode mode);
const char *motionModeName(MotionMode mode);
const char *homingStateName(HomingState state);
const char *homingErrorName(HomingError error);

} // namespace LiF_Motor
