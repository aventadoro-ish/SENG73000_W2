#include <Arduino.h>

#include "pin_definition.h"
#include "Motor.h"
#include "CAN.h"
#include "utils.h"



// -----------------------------------------------------------------------------
// Compile Settings
// -----------------------------------------------------------------------------
#define HOME_ON_STARTUP
#define ENERGIZE_ON_STARTUP
// #define USE_MOTOR_SERIAL
// #define USE_SIMPLIFIED_CAN_PROTOCOL
#define USE_HARDCODED_FLOORS



#ifdef USE_HARDCODED_FLOORS
#define FLOOR1_STEPS    500
#define FLOOR2_STEPS    1000
#define FLOOR3_STEPS    1500
#endif


enum class EC_State : uint8_t {
    INITIALIZE,
    IDLE,
    MOVING,
    DISABLED,
    FAULT
};


EC_State state = EC_State::INITIALIZE;
unsigned long int last_heartbeat_time;
uint8_t target_floor = 255;     // 255 ensures the initial floor request results in move
uint8_t current_floor = 0;      // 0 is internally interpreted as moving


/**
 * @brief Process incoming CAN frame according to the simplified test CAN protocol
 * @param rxMsg received message to process
 */
void process_CAN_msg_simplified_mode(CAN_message_t rxMsg);

/**
 * @brief Process incoming CAN frame according to the proper elevator CAN 
 * protocol.
 * 
 * State transitions handling:
 * 
 *  - any -> `FAULT`
 * 
 *  - not `DISABLED` -> `DISABLED`
 * 
 *  - `DISABLED` -> `IDLE` or `MOVING`
 * @param rxMsg received message to process
 */
void process_CAN_msg_full_mode(CAN_message_t rxMsg);

void send_EC_CAN_frame(bool is_enabled, uint8_t position);

void send_heartbeat();

/// @brief Infinite loop. E-stop is activated.
/// Serial sends "FAULT",
/// CAN sends disabled state and floor position as moving (0).
/// 
/// MCU restart is required to exit the fault mode
/// @param reason (optional) string pointer explaining the reason of the fault
void fault_mode(const char* reason = nullptr);


// -----------------------------------------------------------------------------
// Arduino setup and loop
// -----------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ;
    }
    Serial.println("LiF EC - Setup Started");

    LiF_CAN::setup();
    LiF_Motor::setup();
    LiF_Motor::setupMotorControlTimer();

#ifdef ENERGIZE_ON_STARTUP
    LiF_Motor::energizeCoils();
#endif

#ifdef HOME_ON_STARTUP
    // Non-blocking: tickISR() performs the sequence in the background.
    if (!LiF_Motor::startHoming()) {
        Serial.print("Unable to start homing: ");
        Serial.println(
            LiF_Motor::homingErrorName(LiF_Motor::getHomingError()));
    }
#endif

    last_heartbeat_time = millis();
    state = EC_State::IDLE;

    Serial.println("LiF EC - Setup finished");

    send_heartbeat();           // initializes last_heartbeat_time and informs SC that EC is enabled and ready
}


void loop() {
#ifdef USE_MOTOR_SERIAL
    LiF_Motor_Test::motorSerialTest();
#endif

    // Process incoming CAN messages
    CAN_message_t rxMsg;
    while (LiF_CAN::bus.read(rxMsg)) {
        // CAN Message received
#ifdef USE_SIMPLIFIED_CAN_PROTOCOL
        process_CAN_msg_simplified_mode(rxMsg);
#else
        process_CAN_msg_full_mode(rxMsg);
#endif
    }

    // send "heartbeat"
    if (millis() - last_heartbeat_time >= LiF_CAN::heartbeat_period_ms) {
        send_heartbeat();
    }

    
    // check if in final position
    if (
        state == EC_State::MOVING && 
        LiF_Motor::getTargetPositionSteps() == LiF_Motor::getPositionSteps()
    ) {
        // it is implied that the motor is stopped
        state == EC_State::IDLE;
        send_heartbeat();
    }


}


void process_CAN_msg_simplified_mode(CAN_message_t rxMsg) {
    Serial.printf(
        "Received CAN msg (S): ID:%x, DATA: %x %x %x %x %x %x %x %x\r\n",
        rxMsg.id,
        rxMsg.buf[0],
        rxMsg.buf[1],
        rxMsg.buf[2],
        rxMsg.buf[3],
        rxMsg.buf[4],
        rxMsg.buf[5],
        rxMsg.buf[6],
        rxMsg.buf[7]
    );

    if (rxMsg.buf[0] == 1) {
        DEBUG_PRINTLN("Moving to floor 1");
        LiF_Motor::moveToSteps(500);
    } else if (rxMsg.buf[0] == 2) {
        DEBUG_PRINTLN("Moving to floor 2");
        LiF_Motor::moveToSteps(1000);

    } else if (rxMsg.buf[0] == 3) {
        DEBUG_PRINTLN("Moving to floor 3");
        LiF_Motor::moveToSteps(1500);
        
    } else {
        DEBUG_PRINTLN("Unknown message format");
    }

}


void process_CAN_msg_full_mode(CAN_message_t rxMsg) {
    // filter unwanted messages
    if (
        rxMsg.id == LiF_CAN::FILTER_CC || 
        rxMsg.id == LiF_CAN::FILTER_F1 || 
        rxMsg.id == LiF_CAN::FILTER_F2 || 
        rxMsg.id == LiF_CAN::FILTER_F3
    ) {
        Serial.printf("Received CAN message with ID %x. Ignoring", rxMsg.id);
        return;
    }

    if (rxMsg.id == LiF_CAN::FILTER_CC) {
        // message from the supervisory controller
        bool is_enabled = (rxMsg.buf[0] && 0b00000100) >> 2;
        uint8_t floor_req = (rxMsg.buf[0] && 0b00000011);

        if (floor_req == 0) {
            // illegal floor request -> enter fault mode
            state = EC_State::FAULT;
            fault_mode("Illegal floor request (0)");
        }

        if (is_enabled && state == EC_State::DISABLED) {
            // enable cmd received when EC is disabled
            state == EC_State::IDLE;
        } else if (!is_enabled && state != EC_State::DISABLED) {
            // disable cmd received when EC is "enabled"
            state == EC_State::DISABLED;
            LiF_Motor::stop();
        }

        if (is_enabled && floor_req != current_floor) {
            state == EC_State::MOVING;
            target_floor = floor_req;
            
#ifdef USE_HARDCODED_FLOORS
            switch (target_floor) {
            case 1:     LiF_Motor::moveToSteps(FLOOR1_STEPS); break;
            case 2:     LiF_Motor::moveToSteps(FLOOR2_STEPS); break;
            case 3:     LiF_Motor::moveToSteps(FLOOR3_STEPS); break;
            default:
                fault_mode("Illegal switch case value");
                break;
            }
#else
#error "Dynamic (non-hardcoded) floors are not supported yet
#endif

            // inform SC that we are now moving
            send_heartbeat();
        }
    }
}


void send_EC_CAN_frame(bool is_enabled, uint8_t position) {
    CAN_message_t txMsg;
    txMsg.id = LiF_CAN::TxID;   // EC message id
    txMsg.buf[0] = 
        (is_enabled << 2) |     // enabled bit 
        (position & 0b11);      // floor position

    LiF_CAN::bus.write(txMsg);
}


void send_heartbeat() {
    last_heartbeat_time = millis();
    bool is_enabled =                       // EC is considered enabled when...
        (state != EC_State::DISABLED)   &&  // state is not DISABLED
        (state != EC_State::FAULT)      &&  // there are no faults, and
        (state != EC_State::INITIALIZE);    // initialization is done
    uint8_t floor_pos;
    if (state == EC_State::MOVING) {
        floor_pos = 0;
    } else {
        // if not moving, assume target floor is reached
        floor_pos = target_floor;
    }
    send_EC_CAN_frame(is_enabled, target_floor);
}


void fault_mode(const char* reason) {
    unsigned long int message_period_ms = 1000;
    unsigned long int last_message_time = millis() - message_period_ms;
    
    LiF_Motor::emergencyStop();
    LiF_Motor::releaseCoils();
    
    for (;;) {
        if (millis() - last_heartbeat_time >= message_period_ms) {
            last_message_time = millis();
            
            if (reason != nullptr) {
                Serial.printf("FAULT MODE: %s\r\n", reason);
            } else {
                Serial.println("FAULT MODE");
            }

            send_EC_CAN_frame(false, 0);
        }
    }
}
