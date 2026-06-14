#include <Arduino.h>
#include <Stepper.h>


namespace LiF_Motor {
    constexpr int STEPS_PER_REVOLUTION = 360 / 1.8;  // 200 steps/revolution

    enum class Direction : int8_t {
        UP = 1,
        DOWN = -1
    };

    enum StepType {
        NOSTEP,                 // 0
        HALFSTEPUP,             // 1
        FULLSTEPUP,             // 2
        FULLSTEPDOWN = -2,      // -2
        HALFSTEPDOWN = -1       // -1
    };

    enum class MotionMode {
        STOPPED,
        MANUAL,
        SWEEP
    };
    
    long selectedRPM = 10;
    long currentStep = 0;
    long sweepTarget = SWEEP_MAX_STEP;

    Direction direction = Direction::UP;
    MotionMode motionMode = MotionMode::STOPPED;
    
    Stepper motor(STEPS_PER_REVOLUTION, IN1, IN2, IN3, IN4);
    
    void setup();

    void release();
    void energize();

    void zero_sequence();

    void set_speed();

    const char *directionName();
    const char *motionModeName();


}