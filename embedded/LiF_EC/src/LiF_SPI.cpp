#include "LiF_SPI.h"
#include "pin_definition.h"

#include <SPI.h>

namespace {

// MCP23S17 supports SPI mode 0 or mode 3.
// Mode 0 avoids the special first-CS-toggle requirement associated with mode 3.
constexpr uint32_t SPI_CLOCK_HZ = 4000000;

SPIClass lifSpi(SPI_MOSI, SPI_MISO, SPI_SCK);
SPISettings lifSpiSettings(SPI_CLOCK_HZ, MSBFIRST, SPI_MODE0);

bool spiInitialized = false;

void deselectAll() {
    // The 74x139 enable input is active-low.
    digitalWrite(PIN_CSEN, HIGH);
}

void selectChannel(uint8_t channel) {
    channel &= 0x03;

    // Change address inputs only while all outputs are disabled.
    deselectAll();

    digitalWrite(PIN_CSA0, (channel & 0x01) ? HIGH : LOW);
    digitalWrite(PIN_CSA1, (channel & 0x02) ? HIGH : LOW);

    // Plenty of margin for the demultiplexer propagation delay and
    // the MCP23S17 chip-select setup time.
    delayMicroseconds(1);

    digitalWrite(PIN_CSEN, LOW);
    delayMicroseconds(1);
}

}  // namespace

namespace LiF_SPI {

    void begin() {
        if (spiInitialized) {
            return;
        }

        pinMode(PIN_CSA0, OUTPUT);
        pinMode(PIN_CSA1, OUTPUT);
        pinMode(PIN_CSEN, OUTPUT);

        // Keep all demultiplexer outputs inactive during startup.
        deselectAll();
        digitalWrite(PIN_CSA0, LOW);
        digitalWrite(PIN_CSA1, LOW);

        lifSpi.begin();
        spiInitialized = true;
    }

    void transfer(
        uint8_t chipSelectChannel,
        const uint8_t* txData,
        uint8_t* rxData,
        size_t length
    ) {
        if (length == 0) {
            return;
        }

        begin();

        lifSpi.beginTransaction(lifSpiSettings);
        selectChannel(chipSelectChannel);

        for (size_t i = 0; i < length; ++i) {
            const uint8_t outgoing = (txData != nullptr) ? txData[i] : 0xFF;
            const uint8_t incoming = lifSpi.transfer(outgoing);

            if (rxData != nullptr) {
                rxData[i] = incoming;
            }
        }

        deselectAll();
        delayMicroseconds(1);
        lifSpi.endTransaction();
    }

    namespace MCP23S17 {

        namespace {

            // BANK = 0 register map.
            constexpr uint8_t REG_IODIRB = 0x01;
            constexpr uint8_t REG_IOCON  = 0x0A;
            constexpr uint8_t REG_GPIOB  = 0x13;
            constexpr uint8_t REG_OLATB  = 0x15;

            // HAEN remains disabled, so the MCP23S17 opcode uses address 000.
            constexpr uint8_t OPCODE_WRITE = 0x40;
            constexpr uint8_t OPCODE_READ  = 0x41;

            uint8_t selectedChannel = LCD_EXPANDER_CHANNEL;
            uint8_t portBOutputShadow = 0x00;

        }  // namespace

        bool begin(uint8_t chipSelectChannel) {
            LiF_SPI::begin();

            selectedChannel = chipSelectChannel & 0x03;

            // BANK=0, sequential operation enabled, HAEN disabled.
            writeRegister(REG_IOCON, 0x00);

            // Set the output latch before changing the pins to outputs, avoiding
            // an unintended pulse on E, RS, or R/W.
            portBOutputShadow = 0x00;
            writeRegister(REG_OLATB, portBOutputShadow);

            // The complete B port is dedicated to the LCD.
            writeRegister(REG_IODIRB, 0x00);

            return readRegister(REG_IODIRB) == 0x00;
        }

        void writeRegister(uint8_t registerAddress, uint8_t value) {
            const uint8_t frame[] = {
                OPCODE_WRITE,
                registerAddress,
                value
            };

            LiF_SPI::write(selectedChannel, frame, sizeof(frame));
        }

        uint8_t readRegister(uint8_t registerAddress) {
            const uint8_t txFrame[] = {
                OPCODE_READ,
                registerAddress,
                0xFF
            };

            uint8_t rxFrame[sizeof(txFrame)] = {};
            LiF_SPI::transfer(
                selectedChannel,
                txFrame,
                rxFrame,
                sizeof(txFrame)
            );

            return rxFrame[2];
        }

        void writePortB(uint8_t value) {
            portBOutputShadow = value;
            writeRegister(REG_OLATB, portBOutputShadow);
        }

        void updatePortB(uint8_t mask, uint8_t value) {
            portBOutputShadow =
                static_cast<uint8_t>(
                    (portBOutputShadow & static_cast<uint8_t>(~mask)) |
                    (value & mask)
                );

            writeRegister(REG_OLATB, portBOutputShadow);
        }

        uint8_t portBShadow() {
            return portBOutputShadow;
        }

    }  // namespace MCP23S17
    
}  // namespace LiF_SPI
