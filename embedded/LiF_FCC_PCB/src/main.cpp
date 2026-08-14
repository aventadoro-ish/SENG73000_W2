#include <Arduino.h>
#include <Wire.h>
#include "seven_seg.h"

#include "TCA9555.h"


constexpr int I2C_SDA = 8;
constexpr int I2C_SCL = 9;

TCA9555 display(0x21);
TCA9555 button_leds(0x20);


void I2C_scanner();


void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ;
    }
    Serial.println("LiF FCC - Setup Started");

    while (1) {
        if (Serial.available()) {
            Serial.print(Serial.read());
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
}

void loop() {
    int buttons = button_leds.read8(0);
    bool is_pressed = false;
    for (int i = 0; i < 8; i++) {
        if ((buttons & 1 << i) > 0) {
            display.write16(mapNumber(i + 1));

            is_pressed = true; 
            button_leds.write8(1, 1 << (7 - i));
        }
    }

    if (!is_pressed) {
        display.write16(mapNumber(0));
        button_leds.write8(1, 0);

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

