#include <Arduino.h>
#include <Wire.h>

#include "TCA9555.h"


constexpr int I2C_SDA = 12;
constexpr int I2C_SCL = 17;

//  adjust address if needed
TCA9555 TCA(0b0100001);
void I2C_scanner();


void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ;
    }
    Serial.println("LiF FCC - Setup Started");
    
    // Wire.setSDA(I2C_SDA);
    // Wire.setSCL(I2C_SCL);
    // Wire.begin(I2C_SDA, I2C_SCL);   
    
    // I2C_scanner();
    // Init I2C bus
    // Wire.begin();
    // Wire.setClock(100000);
    
    bool initialized = Wire.begin(I2C_SDA, I2C_SCL, 100000);

    Serial.printf(
        "I2C initialization: %s\n",
        initialized ? "OK" : "FAILED"
    );

    // I2C_scanner();
    
    // TCA.begin();
    // TCA.pinMode16(0x0000);


    // Wire.setTimeOut(5);  // 5 ms instead of the default 50 ms

    Serial.printf("SDA level: %d\n", digitalRead(I2C_SDA));
    Serial.printf("SCL level: %d\n", digitalRead(I2C_SCL));
}

void loop() {
    for (int i = 0; i < 16; i++) {
        int mask = 1 << i;
        TCA.write16(mask);
        delay(200);
    }
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
