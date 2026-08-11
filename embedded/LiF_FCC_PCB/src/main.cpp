#include <Arduino.h>
#include <Wire.h>

#include "TCA9555.h"


constexpr int I2C_SDA = 8;
constexpr int I2C_SCL = 9;

TCA9555 display(0x21);
TCA9555 button_leds(0x20);


void I2C_scanner();
void addDigit1(uint16_t &output, uint8_t pattern);
void addDigit2(uint16_t &output, uint8_t pattern);
void displayNumber(uint8_t number, bool digit1DP = false, bool digit2DP = false);


void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ;
    }
    Serial.println("LiF FCC - Setup Started");

    bool initialized = Wire.begin(I2C_SDA, I2C_SCL, 100000);
    Serial.printf(
        "I2C initialization: %s\n",
        initialized ? "OK" : "FAILED"
    );

    I2C_scanner();
    
    display.begin();
    display.pinMode16(0x0000);
    displayNumber(0);

    button_leds.pinMode16(0x00ff);
    button_leds.write8(1, 0);
    


    // Wire.setTimeOut(5);  // 5 ms instead of the default 50 ms

    Serial.printf("SDA level: %d\n", digitalRead(I2C_SDA));
    Serial.printf("SCL level: %d\n", digitalRead(I2C_SCL));
}

void loop() {
    int buttons = button_leds.read8(0);
    bool is_pressed = false;
    for (int i = 0; i < 8; i++) {
        if ((buttons & 1 << i) > 0) {
            displayNumber(i + 1);
            is_pressed = true; 
            button_leds.write8(1, 1 << (7 - i));
        }
    }

    if (!is_pressed) {
        displayNumber(0);
        button_leds.write8(1, 0);

    }


    // for (int i = 0; i < 16; i++) {
    //     int mask = 1 << i;
    //     display.write16(mask);
    //     delay(200);
    // }

    // for (int i = 0; i < 8; i++) {
    //     int mask = 1 << i;
    //     button_leds.write8(1, mask);
    //     delay(200);
    // }
    // uint8_t deviceCount = 0;
    // uint16_t timeoutCount = 0;
    // uint32_t scanStart = millis();

    // for (uint8_t address = 1; address < 127; address++) {
    //     Wire.beginTransmission(address);
    //     uint8_t result = Wire.endTransmission(true);

    //     if (result == 0) {
    //         Serial.printf("Device found at 0x%02X\n", address);
    //         deviceCount++;
    //     } else if (result == 5) {
    //         timeoutCount++;
    //     }
    // }

    // Serial.printf(
    //     "Scan took %lu ms; devices: %u; timeouts: %u\n",
    //     millis() - scanStart,
    //     deviceCount,
    //     timeoutCount
    // );

    // delay(2000);
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


// Logical segment positions used by the lookup table
constexpr uint8_t SEG_A = (1U << 0);
constexpr uint8_t SEG_B = (1U << 1);
constexpr uint8_t SEG_C = (1U << 2);
constexpr uint8_t SEG_D = (1U << 3);
constexpr uint8_t SEG_E = (1U << 4);
constexpr uint8_t SEG_F = (1U << 5);
constexpr uint8_t SEG_G = (1U << 6);

// Segment patterns for digits 0-9
const uint8_t digitPatterns[10] = {
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,         // 0
    SEG_B | SEG_C,                                         // 1
    SEG_A | SEG_B | SEG_D | SEG_E | SEG_G,                 // 2
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_G,                 // 3
    SEG_B | SEG_C | SEG_F | SEG_G,                         // 4
    SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,                 // 5
    SEG_A | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,         // 6
    SEG_A | SEG_B | SEG_C,                                 // 7
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G, // 8
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G          // 9
};

void addDigit1(uint16_t &output, uint8_t pattern) {
    if (pattern & SEG_A) output |= (1U << 13); // Port B pin 5
    if (pattern & SEG_B) output |= (1U << 12); // Port B pin 4
    if (pattern & SEG_C) output |= (1U << 2);  // Port A pin 2
    if (pattern & SEG_D) output |= (1U << 1);  // Port A pin 1
    if (pattern & SEG_E) output |= (1U << 0);  // Port A pin 0
    if (pattern & SEG_F) output |= (1U << 15); // Port B pin 7
    if (pattern & SEG_G) output |= (1U << 14); // Port B pin 6
}

void addDigit2(uint16_t &output, uint8_t pattern) {
    if (pattern & SEG_A) output |= (1U << 10); // Port B pin 2
    if (pattern & SEG_B) output |= (1U << 9);  // Port B pin 1
    if (pattern & SEG_C) output |= (1U << 7);  // Port A pin 7
    if (pattern & SEG_D) output |= (1U << 5);  // Port A pin 5
    if (pattern & SEG_E) output |= (1U << 4);  // Port A pin 4
    if (pattern & SEG_F) output |= (1U << 11); // Port B pin 3
    if (pattern & SEG_G) output |= (1U << 6);  // Port A pin 6
}

void displayNumber(uint8_t number, bool digit1DP, bool digit2DP) {
    // Keep only the last two digits if number is greater than 99
    number %= 100;

    uint8_t digit1 = number / 10;
    uint8_t digit2 = number % 10;

    uint16_t output = 0;

    addDigit1(output, digitPatterns[digit1]);
    addDigit2(output, digitPatterns[digit2]);

    if (digit1DP) output |= (1U << 3); // Port A pin 3
    if (digit2DP) output |= (1U << 8); // Port B pin 0

    display.write16(output);
}
