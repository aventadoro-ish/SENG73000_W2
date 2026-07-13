#pragma once

#include <Arduino.h>

#include "pin_definition.h"


namespace LiF_SPI {

// 74x139 output used by the LCD's MCP23S17.
constexpr uint8_t LCD_EXPANDER_CHANNEL = 0;

/**
 * @brief Initialize the STM32 SPI peripheral and the external 74x139
 * chip-select demultiplexer.
 */
void begin();

/**
 * @brief Perform one complete SPI transaction.
 *
 * @param chipSelectChannel 74x139 output number, 0 to 3.
 * @param txData Data to transmit. Pass nullptr to transmit 0xFF bytes.
 * @param rxData Receive buffer. Pass nullptr to discard received bytes.
 * @param length Number of bytes to transfer.
 */
void transfer(
    uint8_t chipSelectChannel,
    const uint8_t* txData,
    uint8_t* rxData,
    size_t length
);

/**
 * @brief Convenience wrapper for a write-only transaction.
 */
inline void write(
    uint8_t chipSelectChannel,
    const uint8_t* data,
    size_t length
) {
    transfer(chipSelectChannel, data, nullptr, length);
}

namespace MCP23S17 {

/**
 * @brief Initialize one MCP23S17 selected through the 74x139.
 *
 * This driver uses MCP23S17 hardware address 0 and leaves HAEN disabled,
 * because each device is already selected by a separate demultiplexer output.
 *
 * @return true when the IODIRB configuration can be read back correctly.
 */
bool begin(uint8_t chipSelectChannel = LCD_EXPANDER_CHANNEL);

void writeRegister(uint8_t registerAddress, uint8_t value);
uint8_t readRegister(uint8_t registerAddress);

/**
 * @brief Write all eight PORTB output latches.
 */
void writePortB(uint8_t value);

/**
 * @brief Modify selected PORTB output bits while preserving the other bits.
 */
void updatePortB(uint8_t mask, uint8_t value);

uint8_t portBShadow();

}  // namespace MCP23S17
}  // namespace LiF_SPI
