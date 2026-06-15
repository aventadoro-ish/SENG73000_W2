#include "Motor.h"

#include <cmath>
#include <cstdint>
#include <limits>

namespace LiF_Motor {
namespace {

constexpr int32_t Q16_ONE = 1L << 16;
constexpr uint64_t STEP_ACCUMULATOR_THRESHOLD =
    static_cast<uint64_t>(CONTROL_TICK_HZ) * Q16_ONE;
constexpr uint32_t LIMIT_DEBOUNCE_TICKS_RAW =
    (CONTROL_TICK_HZ * LIMIT_DEBOUNCE_MS + 999U) / 1000U;
constexpr uint32_t LIMIT_DEBOUNCE_TICKS =
    (LIMIT_DEBOUNCE_TICKS_RAW == 0U) ? 1U : LIMIT_DEBOUNCE_TICKS_RAW;
constexpr int32_t ACCEL_PER_TICK_Q16_RAW = static_cast<int32_t>(
    (MAX_ACCEL_STEPS_S2 * static_cast<float>(Q16_ONE)) /
    static_cast<float>(CONTROL_TICK_HZ));
constexpr int32_t ACCEL_PER_TICK_Q16 =
    (ACCEL_PER_TICK_Q16_RAW < 1) ? 1 : ACCEL_PER_TICK_Q16_RAW;

struct LimitDebouncer {
    bool stableActive = false;
    uint32_t mismatchTicks = 0;
};

volatile bool g_lowerRawActive = false;
volatile bool g_upperRawActive = false;

LimitDebouncer g_lowerDebouncer;
LimitDebouncer g_upperDebouncer;

volatile bool g_coilsEnergized = false;
volatile MotionMode g_motionMode = MotionMode::IDLE;
volatile HomingState g_homingState = HomingState::NOT_HOMED;
volatile HomingError g_homingError = HomingError::NONE;
volatile bool g_homed = false;

volatile StepMode g_stepMode = StepMode::FULL_STEP;
// Internal position unit is always one electrical half-step.
volatile int32_t g_positionHalfSteps = 0;
volatile int32_t g_targetPositionHalfSteps = 0;
volatile int32_t g_travelRangeHalfSteps = 0;

// Q16.16 steps/second. Positive is upward.
volatile int32_t g_currentSpeedQ16 = 0;
volatile int32_t g_commandSpeedQ16 = 0;
volatile int32_t g_positionMaxSpeedQ16 = 0;

uint64_t g_stepAccumulator = 0;
uint8_t g_phaseIndex = 0;
int32_t g_homingLegStartHalfSteps = 0;
int32_t g_upperHitHalfSteps = 0;

volatile bool g_tofValid = false;
volatile int32_t g_tofPositionMicrometres = 0;
volatile uint32_t g_tofSampleTimeMs = 0;

class InterruptLock {
public:
    InterruptLock() : primask_(__get_PRIMASK()) {
        __disable_irq();
    }

    ~InterruptLock() {
        if (primask_ == 0U) {
            __enable_irq();
        }
    }

    InterruptLock(const InterruptLock &) = delete;
    InterruptLock &operator=(const InterruptLock &) = delete;

private:
    uint32_t primask_;
};

int32_t floatToQ16(float value) {
    const double scaled = static_cast<double>(value) * Q16_ONE;
    if (scaled >= static_cast<double>(std::numeric_limits<int32_t>::max())) {
        return std::numeric_limits<int32_t>::max();
    }
    if (scaled <= static_cast<double>(std::numeric_limits<int32_t>::min())) {
        return std::numeric_limits<int32_t>::min();
    }
    return static_cast<int32_t>(std::lround(scaled));
}

float q16ToFloat(int32_t value) {
    return static_cast<float>(value) / static_cast<float>(Q16_ONE);
}

int32_t clampAbsSpeedQ16(float speedStepsPerSecond) {
    const float bounded = constrain(speedStepsPerSecond,
                                    -MAX_COMMAND_SPEED_STEPS_S,
                                    MAX_COMMAND_SPEED_STEPS_S);
    return floatToQ16(bounded);
}

bool pinIsActive(uint32_t pin) {
    return digitalRead(pin) == LIMIT_ACTIVE_LEVEL;
}

void lowerLimitISR() {
    g_lowerRawActive = pinIsActive(LOWER_LIMIT_PIN);
}

void upperLimitISR() {
    g_upperRawActive = pinIsActive(UPPER_LIMIT_PIN);
}

void updateDebouncer(LimitDebouncer &debouncer, bool rawActive) {
    if (rawActive == debouncer.stableActive) {
        debouncer.mismatchTicks = 0;
        return;
    }

    if (++debouncer.mismatchTicks >= LIMIT_DEBOUNCE_TICKS) {
        debouncer.stableActive = rawActive;
        debouncer.mismatchTicks = 0;
    }
}

enum class WindingState : int8_t {
    NEGATIVE = -1,
    OFF = 0,
    POSITIVE = 1
};

void driveWinding(uint32_t positivePin,
                  uint32_t negativePin,
                  WindingState state) {
    switch (state) {
        case WindingState::POSITIVE:
            digitalWrite(positivePin, HIGH);
            digitalWrite(negativePin, LOW);
            break;
        case WindingState::NEGATIVE:
            digitalWrite(positivePin, LOW);
            digitalWrite(negativePin, HIGH);
            break;
        case WindingState::OFF:
        default:
            // Both bridge inputs LOW removes differential winding voltage.
            digitalWrite(positivePin, LOW);
            digitalWrite(negativePin, LOW);
            break;
    }
}

void driveBridge(WindingState windingA, WindingState windingB) {
    driveWinding(IN1, IN2, windingA);
    driveWinding(IN3, IN4, windingB);
}

void applyCurrentPhase() {
    if (!g_coilsEnergized) {
        driveBridge(WindingState::OFF, WindingState::OFF);
        return;
    }

    // Eight-state bipolar half-step sequence. Even indices energize both
    // windings and are also used by FULL_STEP mode:
    // 0 A+ B+   1 A0 B+   2 A- B+   3 A- B0
    // 4 A- B-   5 A0 B-   6 A+ B-   7 A+ B0
    switch (g_phaseIndex & 0x07U) {
        case 0:
            driveBridge(WindingState::POSITIVE, WindingState::POSITIVE);
            break;
        case 1:
            driveBridge(WindingState::OFF, WindingState::POSITIVE);
            break;
        case 2:
            driveBridge(WindingState::NEGATIVE, WindingState::POSITIVE);
            break;
        case 3:
            driveBridge(WindingState::NEGATIVE, WindingState::OFF);
            break;
        case 4:
            driveBridge(WindingState::NEGATIVE, WindingState::NEGATIVE);
            break;
        case 5:
            driveBridge(WindingState::OFF, WindingState::NEGATIVE);
            break;
        case 6:
            driveBridge(WindingState::POSITIVE, WindingState::NEGATIVE);
            break;
        case 7:
        default:
            driveBridge(WindingState::POSITIVE, WindingState::OFF);
            break;
    }
}

void forceStopped(MotionMode nextMode = MotionMode::IDLE) {
    g_currentSpeedQ16 = 0;
    g_commandSpeedQ16 = 0;
    g_stepAccumulator = 0;
    g_motionMode = nextMode;
}

void failHoming(HomingError error) {
    forceStopped(MotionMode::IDLE);
    g_homingState = HomingState::FAILED;
    g_homingError = error;
    g_homed = false;
}

int32_t absQ16(int32_t value) {
    if (value == std::numeric_limits<int32_t>::min()) {
        return std::numeric_limits<int32_t>::max();
    }
    return (value < 0) ? -value : value;
}

int32_t signOf(int32_t value) {
    return (value > 0) - (value < 0);
}

void updateCurrentSpeed(int32_t desiredSpeedQ16) {
    const int32_t oldSpeed = g_currentSpeedQ16;

    if (g_currentSpeedQ16 < desiredSpeedQ16) {
        const int64_t accelerated =
            static_cast<int64_t>(g_currentSpeedQ16) + ACCEL_PER_TICK_Q16;
        g_currentSpeedQ16 = static_cast<int32_t>(
            (accelerated > desiredSpeedQ16) ? desiredSpeedQ16 : accelerated);
    } else if (g_currentSpeedQ16 > desiredSpeedQ16) {
        const int64_t decelerated =
            static_cast<int64_t>(g_currentSpeedQ16) - ACCEL_PER_TICK_Q16;
        g_currentSpeedQ16 = static_cast<int32_t>(
            (decelerated < desiredSpeedQ16) ? desiredSpeedQ16 : decelerated);
    }

    // Do not carry a fractional step from one direction into the other.
    if (signOf(oldSpeed) != signOf(g_currentSpeedQ16) ||
        g_currentSpeedQ16 == 0) {
        g_stepAccumulator = 0;
    }
}

uint32_t integerSqrt64(uint64_t value) {
    // Restoring integer square root. The result is floor(sqrt(value)).
    uint64_t result = 0;
    uint64_t bit = uint64_t{1} << 62;

    while (bit > value) {
        bit >>= 2;
    }

    while (bit != 0) {
        if (value >= result + bit) {
            value -= result + bit;
            result = (result >> 1) + bit;
        } else {
            result >>= 1;
        }
        bit >>= 2;
    }

    return static_cast<uint32_t>(result);
}

int32_t positionPlannerSpeedQ16() {
    const int32_t remainingHalfSteps =
        g_targetPositionHalfSteps - g_positionHalfSteps;

    if (remainingHalfSteps == 0) {
        return 0;
    }

    const int32_t desiredDirection = signOf(remainingHalfSteps);
    // Convert half-step distance to Q16.16 physical full-step distance.
    const uint64_t remainingDistanceQ16 =
        static_cast<uint64_t>(std::abs(remainingHalfSteps)) *
        static_cast<uint64_t>(Q16_ONE) / 2ULL;
    const uint64_t accelerationQ16 =
        static_cast<uint64_t>(floatToQ16(MAX_ACCEL_STEPS_S2));

    // Velocity envelope v = sqrt(2*a*d). a is full-steps/s^2 and d is
    // physical full steps, so the result is full-steps/s in Q16.16.
    const uint64_t radicand =
        2ULL * accelerationQ16 * remainingDistanceQ16;
    const int32_t brakingEnvelopeQ16 = static_cast<int32_t>(
        integerSqrt64(radicand));
    const int32_t allowedSpeedQ16 =
        min(absQ16(g_positionMaxSpeedQ16), brakingEnvelopeQ16);

    return desiredDirection * allowedSpeedQ16;
}

int32_t halfStepIncrementForMode() {
    return (g_stepMode == StepMode::HALF_STEP) ? 1 : 2;
}

bool stepWouldCrossPositionTarget(int32_t direction) {
    if (g_motionMode != MotionMode::POSITION) {
        return false;
    }

    const int32_t nextPosition =
        g_positionHalfSteps + direction * halfStepIncrementForMode();
    return (direction > 0 && nextPosition > g_targetPositionHalfSteps) ||
           (direction < 0 && nextPosition < g_targetPositionHalfSteps);
}

bool takeOneStep(int32_t direction) {
    if (direction == 0 || !g_coilsEnergized) {
        return false;
    }

    // Raw (not merely debounced) inputs provide immediate hard-limit protection.
    if ((direction > 0 && g_upperRawActive) ||
        (direction < 0 && g_lowerRawActive)) {
        forceStopped(MotionMode::IDLE);
        return false;
    }

    if (stepWouldCrossPositionTarget(direction)) {
        forceStopped(MotionMode::IDLE);
        return false;
    }

    const int32_t electricalDirection = direction * UP_PHASE_DIRECTION;
    const uint8_t phaseIncrement =
        (g_stepMode == StepMode::HALF_STEP) ? 1U : 2U;

    if (electricalDirection > 0) {
        g_phaseIndex = static_cast<uint8_t>(
            (g_phaseIndex + phaseIncrement) & 0x07U);
    } else {
        g_phaseIndex = static_cast<uint8_t>(
            (g_phaseIndex + 8U - phaseIncrement) & 0x07U);
    }

    // Position always follows the public convention: positive is upward.
    g_positionHalfSteps += direction * halfStepIncrementForMode();

    applyCurrentPhase();

    if (g_motionMode == MotionMode::POSITION &&
        g_positionHalfSteps == g_targetPositionHalfSteps) {
        forceStopped(MotionMode::IDLE);
    }

    return true;
}

void processHomingState() {
    if (g_homingState != HomingState::SEEKING_UPPER &&
        g_homingState != HomingState::SEEKING_LOWER) {
        return;
    }

    if (g_lowerDebouncer.stableActive &&
        g_upperDebouncer.stableActive) {
        failHoming(HomingError::BOTH_LIMITS_ACTIVE);
        return;
    }

    if (g_homingState == HomingState::SEEKING_UPPER) {
        if (g_upperRawActive) {
            g_currentSpeedQ16 = 0;
            g_stepAccumulator = 0;
        }

        if (g_upperDebouncer.stableActive) {
            g_upperHitHalfSteps = g_positionHalfSteps;
            g_homingLegStartHalfSteps = g_positionHalfSteps;
            g_homingState = HomingState::SEEKING_LOWER;
            return;
        }

        if (std::abs(g_positionHalfSteps - g_homingLegStartHalfSteps) >
            (2 * HOMING_MAX_LEG_STEPS)) {
            failHoming(HomingError::UPPER_NOT_FOUND);
        }
        return;
    }

    if (g_lowerRawActive) {
        g_currentSpeedQ16 = 0;
        g_stepAccumulator = 0;
    }

    if (g_lowerDebouncer.stableActive) {
        const int32_t measuredRangeHalfSteps =
            g_upperHitHalfSteps - g_positionHalfSteps;

        if (measuredRangeHalfSteps <= 0) {
            failHoming(HomingError::BOTH_LIMITS_ACTIVE);
            return;
        }

        g_travelRangeHalfSteps = measuredRangeHalfSteps;
        g_positionHalfSteps = 0;
        g_targetPositionHalfSteps = 0;
        g_homed = true;
        g_homingState = HomingState::COMPLETE;
        g_homingError = HomingError::NONE;
        forceStopped(MotionMode::IDLE);
        return;
    }

    if (std::abs(g_positionHalfSteps - g_homingLegStartHalfSteps) >
        (2 * HOMING_MAX_LEG_STEPS)) {
        failHoming(HomingError::LOWER_NOT_FOUND);
    }
}

int32_t homingDesiredSpeedQ16() {
    if (g_homingState == HomingState::SEEKING_UPPER) {
        return g_upperRawActive ? 0 : floatToQ16(HOMING_SPEED_STEPS_S);
    }
    if (g_homingState == HomingState::SEEKING_LOWER) {
        return g_lowerRawActive ? 0 : -floatToQ16(HOMING_SPEED_STEPS_S);
    }
    return 0;
}

int32_t determineDesiredSpeedQ16() {
    switch (g_motionMode) {
        case MotionMode::SPEED:
            return g_commandSpeedQ16;

        case MotionMode::POSITION:
            return positionPlannerSpeedQ16();

        case MotionMode::STOPPING:
            return 0;

        case MotionMode::HOMING:
            return homingDesiredSpeedQ16();

        case MotionMode::IDLE:
        default:
            return 0;
    }
}

void cancelCommandIntoActiveLimit(int32_t &desiredSpeedQ16) {
    // During homing, processHomingState() owns the limit response.
    if (g_motionMode == MotionMode::HOMING) {
        return;
    }

    if (g_upperRawActive) {
        // Stop immediately if momentum is still upward. Preserve a command
        // that moves away from the switch, but cancel one that moves into it.
        if (g_currentSpeedQ16 > 0) {
            g_currentSpeedQ16 = 0;
            g_stepAccumulator = 0;
        }
        if (desiredSpeedQ16 > 0) {
            desiredSpeedQ16 = 0;
            forceStopped(MotionMode::IDLE);
            return;
        }
    }

    if (g_lowerRawActive) {
        if (g_currentSpeedQ16 < 0) {
            g_currentSpeedQ16 = 0;
            g_stepAccumulator = 0;
        }
        if (desiredSpeedQ16 < 0) {
            desiredSpeedQ16 = 0;
            forceStopped(MotionMode::IDLE);
        }
    }
}

} // namespace

void setup() {
    InterruptLock lock;

    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);

    pinMode(LOWER_LIMIT_PIN, LIMIT_PIN_MODE);
    pinMode(UPPER_LIMIT_PIN, LIMIT_PIN_MODE);

    g_lowerRawActive = pinIsActive(LOWER_LIMIT_PIN);
    g_upperRawActive = pinIsActive(UPPER_LIMIT_PIN);
    g_lowerDebouncer.stableActive = g_lowerRawActive;
    g_upperDebouncer.stableActive = g_upperRawActive;
    g_lowerDebouncer.mismatchTicks = 0;
    g_upperDebouncer.mismatchTicks = 0;

    g_phaseIndex = 0;
    g_stepMode = StepMode::FULL_STEP;
    g_positionHalfSteps = 0;
    g_targetPositionHalfSteps = 0;
    g_travelRangeHalfSteps = 0;
    g_currentSpeedQ16 = 0;
    g_commandSpeedQ16 = 0;
    g_positionMaxSpeedQ16 = floatToQ16(DEFAULT_MOVE_SPEED_STEPS_S);
    g_stepAccumulator = 0;
    g_coilsEnergized = false;
    g_motionMode = MotionMode::IDLE;
    g_homingState = HomingState::NOT_HOMED;
    g_homingError = HomingError::NONE;
    g_homed = false;

    applyCurrentPhase();

    attachInterrupt(digitalPinToInterrupt(LOWER_LIMIT_PIN),
                    lowerLimitISR,
                    CHANGE);
    attachInterrupt(digitalPinToInterrupt(UPPER_LIMIT_PIN),
                    upperLimitISR,
                    CHANGE);
}

void tickISR() {
    const bool lowerRaw = g_lowerRawActive;
    const bool upperRaw = g_upperRawActive;
    updateDebouncer(g_lowerDebouncer, lowerRaw);
    updateDebouncer(g_upperDebouncer, upperRaw);

    if (!g_coilsEnergized) {
        g_currentSpeedQ16 = 0;
        g_stepAccumulator = 0;
        if (g_motionMode == MotionMode::HOMING) {
            failHoming(HomingError::COILS_RELEASED);
        }
        return;
    }

    processHomingState();

    int32_t desiredSpeedQ16 = determineDesiredSpeedQ16();
    cancelCommandIntoActiveLimit(desiredSpeedQ16);
    updateCurrentSpeed(desiredSpeedQ16);

    if (g_motionMode == MotionMode::STOPPING && g_currentSpeedQ16 == 0) {
        g_motionMode = MotionMode::IDLE;
    }

    if (g_motionMode == MotionMode::POSITION &&
        g_positionHalfSteps == g_targetPositionHalfSteps &&
        g_currentSpeedQ16 == 0) {
        g_motionMode = MotionMode::IDLE;
    }

    if (g_currentSpeedQ16 == 0) {
        return;
    }

    const uint32_t eventRateMultiplier =
        (g_stepMode == StepMode::HALF_STEP) ? 2U : 1U;
    g_stepAccumulator +=
        static_cast<uint64_t>(absQ16(g_currentSpeedQ16)) *
        eventRateMultiplier;

    if (g_stepAccumulator >= STEP_ACCUMULATOR_THRESHOLD) {
        g_stepAccumulator -= STEP_ACCUMULATOR_THRESHOLD;
        takeOneStep(signOf(g_currentSpeedQ16));
    }
}

bool setStepMode(StepMode mode) {
    InterruptLock lock;
    if (g_motionMode != MotionMode::IDLE ||
        g_currentSpeedQ16 != 0 ||
        g_coilsEnergized) {
        return false;
    }

    if (g_stepMode == mode) {
        return true;
    }

    g_stepMode = mode;
    g_phaseIndex = 0;

    // FULL_STEP cannot represent an odd half-step coordinate. With the coils
    // released, normalize the software coordinate to the nearest full-step
    // boundary toward zero before the rotor is energized again.
    if (mode == StepMode::FULL_STEP) {
        g_positionHalfSteps = (g_positionHalfSteps / 2) * 2;
    }
    g_targetPositionHalfSteps = g_positionHalfSteps;

    g_homed = false;
    g_homingState = HomingState::NOT_HOMED;
    g_homingError = HomingError::NONE;
    g_travelRangeHalfSteps = 0;
    return true;
}

StepMode getStepMode() {
    InterruptLock lock;
    return g_stepMode;
}

void energizeCoils() {
    InterruptLock lock;
    g_coilsEnergized = true;
    applyCurrentPhase();
}

void releaseCoils() {
    InterruptLock lock;
    forceStopped(MotionMode::IDLE);
    g_coilsEnergized = false;
    applyCurrentPhase();

    if (g_homingState == HomingState::SEEKING_UPPER ||
        g_homingState == HomingState::SEEKING_LOWER) {
        g_homingState = HomingState::FAILED;
        g_homingError = HomingError::COILS_RELEASED;
        g_homed = false;
    }
}

bool coilsAreEnergized() {
    InterruptLock lock;
    return g_coilsEnergized;
}

bool setTargetSpeedSteps(float stepsPerSecond) {
    if (!std::isfinite(stepsPerSecond) ||
        std::fabs(stepsPerSecond) > MAX_COMMAND_SPEED_STEPS_S) {
        return false;
    }

    InterruptLock lock;
    if ((stepsPerSecond > 0.0f && g_upperRawActive) ||
        (stepsPerSecond < 0.0f && g_lowerRawActive)) {
        return false;
    }

    g_commandSpeedQ16 = clampAbsSpeedQ16(stepsPerSecond);
    g_motionMode = (g_commandSpeedQ16 == 0)
                       ? MotionMode::STOPPING
                       : MotionMode::SPEED;
    return true;
}

bool setTargetSpeedMm(float mmPerSecond) {
    if (MM_PER_STEP <= 0.0f) {
        return false;
    }
    return setTargetSpeedSteps(mmPerSecond / MM_PER_STEP);
}

float getTargetSpeedSteps() {
    InterruptLock lock;
    return q16ToFloat(g_commandSpeedQ16);
}

float getTargetSpeedMm() {
    return getTargetSpeedSteps() * MM_PER_STEP;
}

float getCurrentSpeedSteps() {
    InterruptLock lock;
    return q16ToFloat(g_currentSpeedQ16);
}

float getCurrentSpeedMm() {
    return getCurrentSpeedSteps() * MM_PER_STEP;
}

bool moveToSteps(int32_t targetFullSteps,
                 float maxSpeedFullStepsPerSecond) {
    const int64_t targetHalfSteps =
        static_cast<int64_t>(targetFullSteps) * 2LL;
    if (targetHalfSteps > std::numeric_limits<int32_t>::max() ||
        targetHalfSteps < std::numeric_limits<int32_t>::min()) {
        return false;
    }
    return moveToHalfSteps(static_cast<int32_t>(targetHalfSteps),
                           maxSpeedFullStepsPerSecond);
}

bool moveToHalfSteps(int32_t targetHalfSteps,
                     float maxSpeedFullStepsPerSecond) {
    if (!std::isfinite(maxSpeedFullStepsPerSecond) ||
        maxSpeedFullStepsPerSecond <= 0.0f ||
        maxSpeedFullStepsPerSecond > MAX_COMMAND_SPEED_STEPS_S) {
        return false;
    }

    InterruptLock lock;
    if (g_stepMode == StepMode::FULL_STEP &&
        (targetHalfSteps & 0x01) != 0) {
        return false;
    }

    if (g_homed &&
        (targetHalfSteps < 0 ||
         targetHalfSteps > g_travelRangeHalfSteps)) {
        return false;
    }

    if ((targetHalfSteps > g_positionHalfSteps && g_upperRawActive) ||
        (targetHalfSteps < g_positionHalfSteps && g_lowerRawActive)) {
        return false;
    }

    g_commandSpeedQ16 = 0;
    g_targetPositionHalfSteps = targetHalfSteps;
    g_positionMaxSpeedQ16 = floatToQ16(maxSpeedFullStepsPerSecond);
    g_motionMode = (targetHalfSteps == g_positionHalfSteps)
                       ? MotionMode::STOPPING
                       : MotionMode::POSITION;
    return true;
}

bool moveToMm(float targetMm, float maxSpeedMmPerSecond) {
    if (!std::isfinite(targetMm) || !std::isfinite(maxSpeedMmPerSecond) ||
        MM_PER_STEP <= 0.0f) {
        return false;
    }

    const double targetHalfSteps =
        static_cast<double>(targetMm) * 2.0 /
        static_cast<double>(MM_PER_STEP);
    if (targetHalfSteps > std::numeric_limits<int32_t>::max() ||
        targetHalfSteps < std::numeric_limits<int32_t>::min()) {
        return false;
    }

    return moveToHalfSteps(
        static_cast<int32_t>(std::lround(targetHalfSteps)),
        maxSpeedMmPerSecond / MM_PER_STEP);
}

void stop() {
    InterruptLock lock;
    g_commandSpeedQ16 = 0;
    g_motionMode = MotionMode::STOPPING;

    if (g_homingState == HomingState::SEEKING_UPPER ||
        g_homingState == HomingState::SEEKING_LOWER) {
        g_homingState = HomingState::NOT_HOMED;
        g_homingError = HomingError::NONE;
        g_homed = false;
    }
}

void emergencyStop() {
    InterruptLock lock;
    forceStopped(MotionMode::IDLE);

    if (g_homingState == HomingState::SEEKING_UPPER ||
        g_homingState == HomingState::SEEKING_LOWER) {
        g_homingState = HomingState::NOT_HOMED;
        g_homingError = HomingError::NONE;
        g_homed = false;
    }
}

int32_t getPositionSteps() {
    return getPositionHalfSteps() / 2;
}

int32_t getPositionHalfSteps() {
    InterruptLock lock;
    return g_positionHalfSteps;
}

float getPositionStepsExact() {
    return static_cast<float>(getPositionHalfSteps()) * 0.5f;
}

float getPositionMm() {
    return getPositionStepsExact() * MM_PER_STEP;
}

int32_t getTargetPositionSteps() {
    return getTargetPositionHalfSteps() / 2;
}

int32_t getTargetPositionHalfSteps() {
    InterruptLock lock;
    return g_targetPositionHalfSteps;
}

float getTargetPositionMm() {
    return static_cast<float>(getTargetPositionHalfSteps()) *
           (MM_PER_STEP * 0.5f);
}

bool setCurrentPositionSteps(int32_t positionFullSteps) {
    const int64_t positionHalfSteps =
        static_cast<int64_t>(positionFullSteps) * 2LL;
    if (positionHalfSteps > std::numeric_limits<int32_t>::max() ||
        positionHalfSteps < std::numeric_limits<int32_t>::min()) {
        return false;
    }
    return setCurrentPositionHalfSteps(
        static_cast<int32_t>(positionHalfSteps));
}

bool setCurrentPositionHalfSteps(int32_t positionHalfSteps) {
    InterruptLock lock;
    if (g_currentSpeedQ16 != 0 || g_motionMode != MotionMode::IDLE) {
        return false;
    }
    if (g_stepMode == StepMode::FULL_STEP &&
        (positionHalfSteps & 0x01) != 0) {
        return false;
    }

    g_positionHalfSteps = positionHalfSteps;
    g_targetPositionHalfSteps = positionHalfSteps;
    return true;
}

bool setCurrentPositionMm(float positionMm) {
    if (!std::isfinite(positionMm) || MM_PER_STEP <= 0.0f) {
        return false;
    }

    const double positionHalfSteps =
        static_cast<double>(positionMm) * 2.0 /
        static_cast<double>(MM_PER_STEP);
    if (positionHalfSteps > std::numeric_limits<int32_t>::max() ||
        positionHalfSteps < std::numeric_limits<int32_t>::min()) {
        return false;
    }

    return setCurrentPositionHalfSteps(
        static_cast<int32_t>(std::lround(positionHalfSteps)));
}

bool startHoming() {
    InterruptLock lock;
    if (!g_coilsEnergized) {
        g_homingState = HomingState::FAILED;
        g_homingError = HomingError::COILS_RELEASED;
        g_homed = false;
        return false;
    }

    if (g_lowerDebouncer.stableActive &&
        g_upperDebouncer.stableActive) {
        g_homingState = HomingState::FAILED;
        g_homingError = HomingError::BOTH_LIMITS_ACTIVE;
        g_homed = false;
        return false;
    }

    g_currentSpeedQ16 = 0;
    g_commandSpeedQ16 = 0;
    g_stepAccumulator = 0;
    g_positionHalfSteps = 0; // temporary coordinate used only during homing
    g_targetPositionHalfSteps = 0;
    g_homingLegStartHalfSteps = 0;
    g_upperHitHalfSteps = 0;
    g_homed = false;
    g_homingError = HomingError::NONE;
    g_homingState = HomingState::SEEKING_UPPER;
    g_motionMode = MotionMode::HOMING;
    return true;
}

void cancelHoming() {
    InterruptLock lock;
    if (g_motionMode == MotionMode::HOMING) {
        forceStopped(MotionMode::IDLE);
    }
    if (g_homingState == HomingState::SEEKING_UPPER ||
        g_homingState == HomingState::SEEKING_LOWER) {
        g_homingState = HomingState::NOT_HOMED;
    }
    g_homingError = HomingError::NONE;
    g_homed = false;
}

bool isHomed() {
    InterruptLock lock;
    return g_homed;
}

bool isHoming() {
    InterruptLock lock;
    return g_motionMode == MotionMode::HOMING;
}

HomingState getHomingState() {
    InterruptLock lock;
    return g_homingState;
}

HomingError getHomingError() {
    InterruptLock lock;
    return g_homingError;
}

int32_t getTravelRangeSteps() {
    return getTravelRangeHalfSteps() / 2;
}

int32_t getTravelRangeHalfSteps() {
    InterruptLock lock;
    return g_travelRangeHalfSteps;
}

float getTravelRangeMm() {
    return static_cast<float>(getTravelRangeHalfSteps()) *
           (MM_PER_STEP * 0.5f);
}

bool lowerLimitIsActive() {
    InterruptLock lock;
    return g_lowerDebouncer.stableActive;
}

bool upperLimitIsActive() {
    InterruptLock lock;
    return g_upperDebouncer.stableActive;
}

void submitTofMeasurementMm(float measuredPositionMm,
                            uint32_t sampleTimeMs) {
    if (!std::isfinite(measuredPositionMm)) {
        invalidateTofMeasurement();
        return;
    }

    const int32_t micrometres = static_cast<int32_t>(
        std::lround(static_cast<double>(measuredPositionMm) * 1000.0));

    InterruptLock lock;
    g_tofPositionMicrometres = micrometres;
    g_tofSampleTimeMs = sampleTimeMs;
    g_tofValid = true;
}

void invalidateTofMeasurement() {
    InterruptLock lock;
    g_tofValid = false;
}

bool getLatestTofMeasurementMm(float &measuredPositionMm,
                               uint32_t &sampleTimeMs) {
    InterruptLock lock;
    if (!g_tofValid) {
        return false;
    }

    measuredPositionMm =
        static_cast<float>(g_tofPositionMicrometres) / 1000.0f;
    sampleTimeMs = g_tofSampleTimeMs;
    return true;
}

MotionMode getMotionMode() {
    InterruptLock lock;
    return g_motionMode;
}

const char *stepModeName(StepMode mode) {
    switch (mode) {
        case StepMode::FULL_STEP:
            return "full";
        case StepMode::HALF_STEP:
            return "half";
        default:
            return "unknown";
    }
}

const char *motionModeName(MotionMode mode) {
    switch (mode) {
        case MotionMode::IDLE:
            return "idle";
        case MotionMode::SPEED:
            return "speed";
        case MotionMode::POSITION:
            return "position";
        case MotionMode::STOPPING:
            return "stopping";
        case MotionMode::HOMING:
            return "homing";
        default:
            return "unknown";
    }
}

const char *homingStateName(HomingState state) {
    switch (state) {
        case HomingState::NOT_HOMED:
            return "not homed";
        case HomingState::SEEKING_UPPER:
            return "seeking upper limit";
        case HomingState::SEEKING_LOWER:
            return "seeking lower limit";
        case HomingState::COMPLETE:
            return "complete";
        case HomingState::FAILED:
            return "failed";
        default:
            return "unknown";
    }
}

const char *homingErrorName(HomingError error) {
    switch (error) {
        case HomingError::NONE:
            return "none";
        case HomingError::BOTH_LIMITS_ACTIVE:
            return "both limits active or invalid measured range";
        case HomingError::UPPER_NOT_FOUND:
            return "upper limit not found";
        case HomingError::LOWER_NOT_FOUND:
            return "lower limit not found";
        case HomingError::COILS_RELEASED:
            return "coils released";
        default:
            return "unknown";
    }
}


// Choose a timer not used by another library/peripheral in this project.
HardwareTimer motorControlTimer(TIM6);


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



} // namespace LiF_Motor




namespace LiF_Motor_Test {

constexpr size_t SERIAL_COMMAND_LENGTH = 96;
char serialCommand[SERIAL_COMMAND_LENGTH];
size_t serialCommandLength = 0;
bool statusWatchEnabled = false;
uint32_t previousWatchPrintMs = 0;
    
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

    
} // namespace LiF_Motor_Test

