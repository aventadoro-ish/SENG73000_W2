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

volatile int32_t g_positionSteps = 0;
volatile int32_t g_targetPositionSteps = 0;
volatile int32_t g_travelRangeSteps = 0;

// Q16.16 steps/second. Positive is upward.
volatile int32_t g_currentSpeedQ16 = 0;
volatile int32_t g_commandSpeedQ16 = 0;
volatile int32_t g_positionMaxSpeedQ16 = 0;

uint64_t g_stepAccumulator = 0;
uint8_t g_phaseIndex = 0;
int32_t g_homingLegStartPosition = 0;
int32_t g_upperHitPosition = 0;

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

void driveBridge(bool aPositive, bool bPositive) {
    digitalWrite(IN1, aPositive ? HIGH : LOW);
    digitalWrite(IN2, aPositive ? LOW : HIGH);
    digitalWrite(IN3, bPositive ? HIGH : LOW);
    digitalWrite(IN4, bPositive ? LOW : HIGH);
}

void applyCurrentPhase() {
    if (!g_coilsEnergized) {
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, LOW);
        return;
    }

    // Bipolar full-step sequence with both windings energized:
    // phase 0: A+, B+
    // phase 1: A-, B+
    // phase 2: A-, B-
    // phase 3: A+, B-
    switch (g_phaseIndex & 0x03U) {
        case 0:
            driveBridge(true, true);
            break;
        case 1:
            driveBridge(false, true);
            break;
        case 2:
            driveBridge(false, false);
            break;
        default:
            driveBridge(true, false);
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
    const int32_t remainingSteps =
        g_targetPositionSteps - g_positionSteps;

    if (remainingSteps == 0) {
        return 0;
    }

    const int32_t desiredDirection = signOf(remainingSteps);
    const uint64_t remainingDistanceQ16 =
        static_cast<uint64_t>(std::abs(remainingSteps)) * Q16_ONE;
    const uint64_t accelerationQ16 =
        static_cast<uint64_t>(floatToQ16(MAX_ACCEL_STEPS_S2));

    // Velocity envelope v = sqrt(2*a*d). Because a and d are both Q16.16,
    // the square root is also Q16.16 steps/s.
    const uint64_t radicand =
        2ULL * accelerationQ16 * remainingDistanceQ16;
    const int32_t brakingEnvelopeQ16 = static_cast<int32_t>(
        integerSqrt64(radicand));
    const int32_t allowedSpeedQ16 =
        min(absQ16(g_positionMaxSpeedQ16), brakingEnvelopeQ16);

    return desiredDirection * allowedSpeedQ16;
}

bool stepWouldCrossPositionTarget(int32_t direction) {
    if (g_motionMode != MotionMode::POSITION) {
        return false;
    }

    return (direction > 0 && g_positionSteps >= g_targetPositionSteps) ||
           (direction < 0 && g_positionSteps <= g_targetPositionSteps);
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
    if (electricalDirection > 0) {
        g_phaseIndex = static_cast<uint8_t>((g_phaseIndex + 1U) & 0x03U);
    } else {
        g_phaseIndex = static_cast<uint8_t>((g_phaseIndex + 3U) & 0x03U);
    }

    // Position always follows the public convention: positive is upward.
    g_positionSteps += direction;

    applyCurrentPhase();

    if (g_motionMode == MotionMode::POSITION &&
        g_positionSteps == g_targetPositionSteps) {
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
            g_upperHitPosition = g_positionSteps;
            g_homingLegStartPosition = g_positionSteps;
            g_homingState = HomingState::SEEKING_LOWER;
            return;
        }

        if (std::abs(g_positionSteps - g_homingLegStartPosition) >
            HOMING_MAX_LEG_STEPS) {
            failHoming(HomingError::UPPER_NOT_FOUND);
        }
        return;
    }

    if (g_lowerRawActive) {
        g_currentSpeedQ16 = 0;
        g_stepAccumulator = 0;
    }

    if (g_lowerDebouncer.stableActive) {
        const int32_t measuredRange =
            g_upperHitPosition - g_positionSteps;

        if (measuredRange <= 0) {
            failHoming(HomingError::BOTH_LIMITS_ACTIVE);
            return;
        }

        g_travelRangeSteps = measuredRange;
        g_positionSteps = 0;
        g_targetPositionSteps = 0;
        g_homed = true;
        g_homingState = HomingState::COMPLETE;
        g_homingError = HomingError::NONE;
        forceStopped(MotionMode::IDLE);
        return;
    }

    if (std::abs(g_positionSteps - g_homingLegStartPosition) >
        HOMING_MAX_LEG_STEPS) {
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
    g_positionSteps = 0;
    g_targetPositionSteps = 0;
    g_travelRangeSteps = 0;
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
        g_positionSteps == g_targetPositionSteps &&
        g_currentSpeedQ16 == 0) {
        g_motionMode = MotionMode::IDLE;
    }

    if (g_currentSpeedQ16 == 0) {
        return;
    }

    g_stepAccumulator += static_cast<uint32_t>(absQ16(g_currentSpeedQ16));

    if (g_stepAccumulator >= STEP_ACCUMULATOR_THRESHOLD) {
        g_stepAccumulator -= STEP_ACCUMULATOR_THRESHOLD;
        takeOneStep(signOf(g_currentSpeedQ16));
    }
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

bool moveToSteps(int32_t targetSteps, float maxSpeedStepsPerSecond) {
    if (!std::isfinite(maxSpeedStepsPerSecond) ||
        maxSpeedStepsPerSecond <= 0.0f ||
        maxSpeedStepsPerSecond > MAX_COMMAND_SPEED_STEPS_S) {
        return false;
    }

    InterruptLock lock;
    if (g_homed &&
        (targetSteps < 0 || targetSteps > g_travelRangeSteps)) {
        return false;
    }

    if ((targetSteps > g_positionSteps && g_upperRawActive) ||
        (targetSteps < g_positionSteps && g_lowerRawActive)) {
        return false;
    }

    g_commandSpeedQ16 = 0;
    g_targetPositionSteps = targetSteps;
    g_positionMaxSpeedQ16 = floatToQ16(maxSpeedStepsPerSecond);
    g_motionMode = (targetSteps == g_positionSteps)
                       ? MotionMode::STOPPING
                       : MotionMode::POSITION;
    return true;
}

bool moveToMm(float targetMm, float maxSpeedMmPerSecond) {
    if (!std::isfinite(targetMm) || !std::isfinite(maxSpeedMmPerSecond) ||
        MM_PER_STEP <= 0.0f) {
        return false;
    }

    const int32_t targetSteps =
        static_cast<int32_t>(std::lround(targetMm / MM_PER_STEP));
    return moveToSteps(targetSteps,
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
    InterruptLock lock;
    return g_positionSteps;
}

float getPositionMm() {
    return static_cast<float>(getPositionSteps()) * MM_PER_STEP;
}

int32_t getTargetPositionSteps() {
    InterruptLock lock;
    return g_targetPositionSteps;
}

float getTargetPositionMm() {
    return static_cast<float>(getTargetPositionSteps()) * MM_PER_STEP;
}

bool setCurrentPositionSteps(int32_t positionSteps) {
    InterruptLock lock;
    if (g_currentSpeedQ16 != 0 || g_motionMode != MotionMode::IDLE) {
        return false;
    }

    g_positionSteps = positionSteps;
    g_targetPositionSteps = positionSteps;
    return true;
}

bool setCurrentPositionMm(float positionMm) {
    if (!std::isfinite(positionMm) || MM_PER_STEP <= 0.0f) {
        return false;
    }
    return setCurrentPositionSteps(
        static_cast<int32_t>(std::lround(positionMm / MM_PER_STEP)));
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
    g_positionSteps = 0; // temporary coordinate used only during homing
    g_targetPositionSteps = 0;
    g_homingLegStartPosition = 0;
    g_upperHitPosition = 0;
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
    InterruptLock lock;
    return g_travelRangeSteps;
}

float getTravelRangeMm() {
    return static_cast<float>(getTravelRangeSteps()) * MM_PER_STEP;
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

} // namespace LiF_Motor
