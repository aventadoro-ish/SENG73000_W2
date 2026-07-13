#include <Arduino.h>
// #include <DiFinders.h>
#include <Wire.h>
// #include <DFRobot_VL53L0X.h>
#include <LCDI2C_Multilingual.h>
#include <VL53L0X.h>

#include "pin_definition.h"
#include "Motor.h"
#include "CAN.h"
#include "utils.h"
#include "LiF_LCD.h"


// -----------------------------------------------------------------------------
// Compile Settings
// -----------------------------------------------------------------------------
// #define HOME_ON_STARTUP
// #define ENERGIZE_ON_STARTUP
#define USE_MOTOR_SERIAL
// #define USE_SIMPLIFIED_CAN_PROTOCOL
#define USE_HARDCODED_FLOORS


// -----------------------------------------------------------------------------
// Global Declarations
// -----------------------------------------------------------------------------
#ifdef USE_HARDCODED_FLOORS
#define FLOOR1_STEPS    100 
#define FLOOR2_STEPS    1200 
#define FLOOR3_STEPS    2250 
#endif

constexpr uint32_t DISPLAY_UPDATE_PERIOD_MS = 500;


enum class EC_State : uint8_t {
    INITIALIZE,
    IDLE,
    MOVING,
    DISABLED,
    FAULT
};


// -----------------------------------------------------------------------------
// Global Variables
// -----------------------------------------------------------------------------
EC_State state = EC_State::INITIALIZE;
unsigned long int last_heartbeat_time;
uint8_t target_floor = 255;     // 255 ensures the initial floor request results in move
uint8_t current_floor = 0;      // 0 is internally interpreted as moving

// TwoWire myWire;
VL53L0X tof_sensor;
unsigned long int last_tof_update;
LCDI2C_Latin_Symbols lcd(0x27, 16, 2);    // I2C address = 0x27


// -----------------------------------------------------------------------------
// Function Declarations
// -----------------------------------------------------------------------------

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

/**
 * @brief Send CAN frame using EC format defined in the elevator protocol.
 * @param is_enabled is EC currently enabled
 * @param position current floor position or 0 for `MOVING` state
 */
void send_EC_CAN_frame(bool is_enabled, uint8_t position);

/**
 * @brief Sends auto-generated heartbeat to CAN based on global variables.
 */
void send_heartbeat();

/// @brief Infinite loop. E-stop is activated.
/// Serial sends "FAULT",
/// CAN sends disabled state and floor position as moving (0).
/// 
/// MCU restart is required to exit the fault mode
/// @param reason (optional) string pointer explaining the reason of the fault
void fault_mode(const char* reason = nullptr);


/**
 * @brief Erases and rewrites info on the LCD screen.
 */
void update_lcd();


// -----------------------------------------------------------------------------
// Arduino setup and loop
// -----------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ;
    }
    Serial.println("LiF EC - Setup Started");

    if (!LiF_LCD::lcd.begin()) {
        Serial.println("LCD initialization failed");

        while (true) {
            delay(1000);
        }
    }
    LiF_LCD::lcd.clear();
    LiF_LCD::lcd.setCursor(0, 0);
    LiF_LCD::lcd.print("LiF EC");

    LiF_LCD::lcd.setCursor(0, 1);
    LiF_LCD::lcd.print("LCD demo");
    delay(2000);

    LiF_LCD::lcd.clear();
    LiF_LCD::lcd.setCursor(0, 0);

    int charCnt = 0;
    while (1) {
        if (Serial.available()) {
            char ch = Serial.read();
            LiF_LCD::lcd.print(ch);
            charCnt++;

            if (charCnt == 16) {
                LiF_LCD::lcd.setCursor(0, 1);
            } else if (charCnt == 32) {
                LiF_LCD::lcd.clear();
                LiF_LCD::lcd.setCursor(0, 0);
                charCnt = 0;
            }
        }
    }


    // myWire = TwoWire(I2C2_SDA, I2C2_SCL);
    // myWire.begin();
    Wire.setSDA(I2C2_SDA);
    Wire.setSCL(I2C2_SCL);

    // Init I2C bus
    Wire.begin();
    Wire.setClock(100000);

    // Serial.println("Scanning...");
    // byte error, address;
    // int nDevices;
    // nDevices = 0;
    // for (address = 1; address < 127; address++)
    // {
    //     // The i2c_scanner uses the return value of
    //     // the Write.endTransmisstion to see if
    //     // a device did acknowledge to the address.
    //     Wire.beginTransmission(address);
    //     error = Wire.endTransmission();
    //     if (error == 0)
    //     {
    //         Serial.print("I2C device found at address 0x");
    //         if (address < 16)
    //             Serial.print("0");
    //         Serial.print(address, HEX);
    //         Serial.println("  !");
    //         nDevices++;
    //     }
    //     else if (error == 4)
    //     {
    //         Serial.print("Unknown error at address 0x");
    //         if (address < 16)
    //             Serial.print("0");
    //         Serial.println(address, HEX);
    //     }
    // }
    // if (nDevices == 0)
    //     Serial.println("No I2C devices found\n");
    // else
    //     Serial.println("done\n");
    // // Set I2C sub-device address
    // tof_sensor.begin(0x29);
    // // Set to Back-to-back mode and high precision mode
    // tof_sensor.setMode(tof_sensor.eSingle, tof_sensor.eHigh);
    // //Laser rangefinder begins to work


    tof_sensor.setTimeout(500);
    if (!tof_sensor.init())
    {
        Serial.println("Failed to detect and initialize sensor!");
        while (1) {}
    }

    // lower the return signal rate limit (default is 0.25 MCPS)
    tof_sensor.setSignalRateLimit(0.1);
    // increase laser pulse periods (defaults are 14 and 10 PCLKs)
    tof_sensor.setVcselPulsePeriod(VL53L0X::VcselPeriodPreRange, 18);
    tof_sensor.setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 14);
    // increase timing budget to 200 ms
    tof_sensor.setMeasurementTimingBudget(200000);
    
    while (1) {
        Serial.print(tof_sensor.readRangeSingleMillimeters());
        if (tof_sensor.timeoutOccurred()) { Serial.print(" TIMEOUT"); }

        Serial.println();
    }

    // Initialize the LCD
    // lcd.init();
    // // Turn on the LCD backlight
    // lcd.backlight();
    
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
    while (LiF_Motor::isHoming()) { 
        ; // wait until homing is finished
    }
#endif

    last_heartbeat_time = millis();
    last_tof_update = millis();
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
        Serial.printf("tStep = %u, cStep = %u, tFlr = %u cFlr = %u", 
            LiF_Motor::getTargetPositionHalfSteps(),
            LiF_Motor::getPositionHalfSteps(),
            target_floor,
            current_floor
        );
        if (state == EC_State::MOVING) {
            Serial.println(" moving");
        } else {
            Serial.println(" not moving");
        }
    }

    
    // check if in final position
    if (
        state == EC_State::MOVING && 
        LiF_Motor::getTargetPositionHalfSteps() == LiF_Motor::getPositionHalfSteps()
    ) {
        Serial.println("Move finished. Now IDLE");
        // it is implied that the motor is stopped
        state = EC_State::IDLE;
        current_floor = target_floor;
        send_heartbeat();
    }

    if (millis() - last_tof_update  >= DISPLAY_UPDATE_PERIOD_MS) {
        last_tof_update = millis();
        update_lcd();
    }

}




// -----------------------------------------------------------------------------
// Function definitions
// -----------------------------------------------------------------------------

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
        Serial.printf("Received CAN message with ID %x. Ignoring\r\n", rxMsg.id);
        return;
    }

    if (rxMsg.id == LiF_CAN::FILTER_SC) {
        Serial.printf("Received CAN message with ID %x from SC. Processing\r\n", rxMsg.id);

        // message from the supervisory controller
        bool is_enabled = (rxMsg.buf[0] & 0b00000100) >> 2;
        uint8_t floor_req = (rxMsg.buf[0] & 0b00000011);

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

        Serial.printf("is_enabled: %u, floor_req %u, current_floor %u\r\n", is_enabled, floor_req, current_floor);
        if (is_enabled && floor_req != current_floor) {
            state = EC_State::MOVING;
            target_floor = floor_req;
            DEBUG_PRINTLN("moving now");
            
#ifdef USE_HARDCODED_FLOORS
            unsigned int tgt_step = 0;
            switch (target_floor) {
            case 1:     tgt_step = FLOOR1_STEPS; break;
            case 2:     tgt_step = FLOOR2_STEPS; break;
            case 3:     tgt_step = FLOOR3_STEPS; break;
            default:
                fault_mode("Illegal switch case value");
                break;
            }

            if (!LiF_Motor::moveToSteps(tgt_step)) {
                Serial.println("Failed to move");
            }
#else
#error "Dynamic (non-hardcoded) floors are not supported yet"
#endif

            // inform SC that we are now moving
            Serial.print("Start Move->");
            send_heartbeat();
        }
    }
}


void send_EC_CAN_frame(bool is_enabled, uint8_t position) {
    // DEBUG_PRINTLN("Sending CAN Frame");
    CAN_message_t txMsg;
    txMsg.id = LiF_CAN::TxID;   // EC message id
    txMsg.buf[0] = 
        (is_enabled << 2) |     // enabled bit 
        (position & 0b11);      // floor position
    txMsg.len = 1;

    LiF_CAN::bus.write(txMsg);
}


void send_heartbeat() {
    // DEBUG_PRINTLN("Sending hearbeat");
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
    send_EC_CAN_frame(is_enabled, floor_pos);
}


void fault_mode(const char* reason) {
    unsigned long int message_period_ms = 1000;
    unsigned long int last_message_time = millis() - message_period_ms;
    
    LiF_Motor::emergencyStop();
    LiF_Motor::releaseCoils();
    
    for (;;) {
        if (millis() - last_heartbeat_time >= message_period_ms) {
            update_lcd();
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



void update_lcd() {
    lcd.clear();
    lcd.home();
    lcd.print("State: ");
    
    switch (state) {
    case EC_State::INITIALIZE:
        lcd.print("init");
        break;
    case EC_State::IDLE:
        lcd.print("idle");
        break;
    case EC_State::MOVING:
        lcd.print("moving");
        break;
    case EC_State::DISABLED:
        lcd.print("disabled");
        break;
    case EC_State::FAULT:
        lcd.print("fault");    
        break;
    default:
        lcd.print("fault");
        fault_mode("invalid EC_State");
        break;
    }
    // tof_sensor.start();
    // float dist = tof_sensor.getDistance();
    // Serial.printf("ToF reads: %f\r\n", dist);
    // Serial.println(dist);

    // tof_sensor.stop();
    
    // tof_sensor.getSignalCount();

    lcd.setCursor(0, 1);
    // lcd.printf("dist: %f", dist);
}