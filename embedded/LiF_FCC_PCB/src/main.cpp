#include <Arduino.h>
#include <Wire.h>
#include "seven_seg.h"

#include "TCA9555.h"
#include <ESP32-TWAI-CAN.hpp>


constexpr int I2C_SDA = 8;
constexpr int I2C_SCL = 9;

constexpr int CAN_TX = 14;
constexpr int CAN_RX = 13;

constexpr uint32_t CAN_CHAR_ID = 0x100;
constexpr uint32_t CAN_BTN_ID = 0x202;

TCA9555 display(0x21);
TCA9555 button_leds(0x20);


void I2C_scanner();


void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ;
    }
    Serial.println("LiF FCC - Setup Started");

    ESP32Can.setPins(CAN_TX, CAN_RX);
    ESP32Can.setSpeed(TWAI_SPEED_100KBPS);

    if (!ESP32Can.begin()) {
        Serial.println("Failed to start CAN!");
        while (true) {
            delay(1000);
        }
    }


    bool initialized = Wire.begin(I2C_SDA, I2C_SCL, 100000);
    Serial.printf(
        "I2C initialization: %s\n",
        initialized ? "OK" : "FAILED"
    );

    I2C_scanner();
    
    display.begin();
    display.pinMode16(0x0000);
    display.write16(mapNumber(0));

    button_leds.pinMode16(0x00ff);
    button_leds.write8(1, 0
    );
    


    // Wire.setTimeOut(5);  // 5 ms instead of the default 50 ms

    Serial.printf("SDA level: %d\n", digitalRead(I2C_SDA));
    Serial.printf("SCL level: %d\n", digitalRead(I2C_SCL));

    Serial.println("Setup finished.");
    Serial.println("CAN started. Type characters to transmit:");

}

void loop() {
    int buttons = button_leds.read8(0);
    bool is_pressed = false;
    for (int i = 0; i < 8; i++) {
        if ((buttons & 1 << i) > 0) {
            display.write16(mapNumber(i + 1));

            is_pressed = true; 
            button_leds.write8(1, 1 << (7 - i));

            CanFrame txFrame = {};
            txFrame.identifier = CAN_BTN_ID;
            txFrame.extd = 0;
            txFrame.rtr = 0;
            txFrame.data_length_code = 1;
            txFrame.data[0] = i + 1;
            if (!ESP32Can.writeFrame(txFrame, 10)) {
                Serial.println("\nCAN transmission failed!");
            }
        }
    }

    // if (!is_pressed) {
    //     display.write16(mapNumber(0));
    //     button_leds.write8(1, 0);
    // }



    // Serial -> CAN
    while (Serial.available() > 0) {
        char character = Serial.read();

        CanFrame txFrame = {};
        txFrame.identifier = CAN_CHAR_ID;
        txFrame.extd = 0;                 // Standard 11-bit CAN ID
        txFrame.rtr = 0;                  // Data frame
        txFrame.data_length_code = 1;
        txFrame.data[0] = static_cast<uint8_t>(character);

        if (!ESP32Can.writeFrame(txFrame, 10)) {
            Serial.println("\nCAN transmission failed!");
        }
    }


    // CAN -> Serial / LEDs
    CanFrame rxFrame = {};

    while (ESP32Can.readFrame(rxFrame, 0)) { // 0 = non-blocking
        if (rxFrame.identifier == CAN_CHAR_ID &&
            !rxFrame.rtr &&
            rxFrame.data_length_code > 0) {

            for (uint8_t i = 0; i < rxFrame.data_length_code; i++) {
                Serial.write(rxFrame.data[i]);
            }

        } else if (rxFrame.identifier > 0x200 &&
            !rxFrame.rtr &&
            rxFrame.data_length_code > 0) {

            int id = rxFrame.identifier - 0x200;
            id = id * 10;
            
            display.write16(mapNumber(id + rxFrame.data[0]));
            is_pressed = true; 
            button_leds.write8(1, 1 << (8 - rxFrame.data[0]));
        }
    }

}



void I2C_scanner() {
    Serial.println("Scanning...");
    byte error, address;
    int nDevices;
    nDevices = 0;
    for (address = 1; address < 127; address++)
    {
        // The i2c_scanner uses the return value of
        // the Write.endTransmisstion to see if
        // a device did acknowledge to the address.
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (error == 0)
        {
            Serial.print("I2C device found at address 0x");
            if (address < 16)
                Serial.print("0");
            Serial.print(address, HEX);
            Serial.println("  !");
            nDevices++;
        }
        else if (error == 4)
        {
            Serial.print("Unknown error at address 0x");
            if (address < 16)
                Serial.print("0");
            Serial.println(address, HEX);
        }
    }
    if (nDevices == 0)
        Serial.println("No I2C devices found\n");
    else
        Serial.println("done\n");
}

