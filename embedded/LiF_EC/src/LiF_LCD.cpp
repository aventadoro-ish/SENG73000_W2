#include "LiF_LCD.h"

#include "LiF_SPI.h"

namespace LiF_LCD {

namespace {

// MCP23S17 PORTB bit assignments.
constexpr uint8_t PIN_DB7 = 1U << 0;
constexpr uint8_t PIN_DB6 = 1U << 1;
constexpr uint8_t PIN_DB5 = 1U << 2;
constexpr uint8_t PIN_DB4 = 1U << 3;
constexpr uint8_t PIN_E   = 1U << 5;
constexpr uint8_t PIN_RW  = 1U << 6;
constexpr uint8_t PIN_RS  = 1U << 7;

// ST7066U commands.
constexpr uint8_t CMD_CLEAR_DISPLAY = 0x01;
constexpr uint8_t CMD_RETURN_HOME   = 0x02;
constexpr uint8_t CMD_ENTRY_MODE    = 0x06;
constexpr uint8_t CMD_DISPLAY_OFF   = 0x08;
constexpr uint8_t CMD_DISPLAY_ON    = 0x0C;
constexpr uint8_t CMD_FUNCTION_SET  = 0x28;  // 4-bit, 2-line, 5x8 font.
constexpr uint8_t CMD_SET_DDRAM     = 0x80;

constexpr uint8_t ROW_OFFSETS[] = {
    0x00,
    0x40
};

/**
 * @brief Convert a conventional four-bit LCD nibble into this PCB's reversed MCP23S17 layout.
 *
 * Nibble
 *   nibble bit 3 -> DB7
 *   nibble bit 2 -> DB6
 *   nibble bit 1 -> DB5
 *   nibble bit 0 -> DB4
 *
 * MCP23S17:
 *
 *   GPB0 -> DB7
 *   GPB1 -> DB6
 *   GPB2 -> DB5
 *   GPB3 -> DB4
 */
uint8_t mapNibbleToPortB(uint8_t nibble) {
    nibble &= 0x0F;

    uint8_t result = 0;

    if (nibble & 0x08) result |= PIN_DB7;
    if (nibble & 0x04) result |= PIN_DB6;
    if (nibble & 0x02) result |= PIN_DB5;
    if (nibble & 0x01) result |= PIN_DB4;

    return result;
}

}  // namespace

LCD16x2 lcd;

bool LCD16x2::begin() {
    initialized_ = false;
    column_ = 0;
    row_ = 0;

    if (!LiF_SPI::MCP23S17::begin(
            LiF_SPI::LCD_EXPANDER_CHANNEL
        )) {
        return false;
    }

    // Keep DB4-DB7, E, R/W and RS low while the LCD powers up.
    // R/W remains low permanently because this first version uses
    // fixed instruction delays instead of reading the busy flag.
    LiF_SPI::MCP23S17::writePortB(0x00);

    delay(50);

    // ST7066U 4-bit initialization:
    // 1. Send the upper nibble of an 8-bit function-set command once.
    // 2. Send 0x28 twice as complete 4-bit commands.
    writeNibble(0x03, false);
    delayMicroseconds(50);

    command(CMD_FUNCTION_SET);
    command(CMD_FUNCTION_SET);

    command(CMD_DISPLAY_OFF);
    clear();
    command(CMD_ENTRY_MODE);
    command(CMD_DISPLAY_ON);

    initialized_ = true;
    setCursor(0, 0);

    return true;
}

void LCD16x2::clear() {
    command(CMD_CLEAR_DISPLAY);
    column_ = 0;
    row_ = 0;
}

void LCD16x2::home() {
    command(CMD_RETURN_HOME);
    column_ = 0;
    row_ = 0;
}

void LCD16x2::setCursor(uint8_t column, uint8_t row) {
    if (row >= ROW_COUNT) {
        row = ROW_COUNT - 1;
    }

    if (column >= COLUMN_COUNT) {
        column = COLUMN_COUNT - 1;
    }

    command(
        static_cast<uint8_t>(
            CMD_SET_DDRAM |
            (ROW_OFFSETS[row] + column)
        )
    );

    column_ = column;
    row_ = row;
}

void LCD16x2::setDisplayEnabled(bool enabled) {
    command(enabled ? CMD_DISPLAY_ON : CMD_DISPLAY_OFF);
}

size_t LCD16x2::write(uint8_t value) {
    if (!initialized_) {
        return 0;
    }

    if (value == '\r') {
        return 1;
    }

    if (value == '\n') {
        setCursor(0, static_cast<uint8_t>((row_ + 1) % ROW_COUNT));
        return 1;
    }

    if (column_ >= COLUMN_COUNT) {
        setCursor(0, static_cast<uint8_t>((row_ + 1) % ROW_COUNT));
    }

    send(value, true);
    ++column_;

    return 1;
}

void LCD16x2::command(uint8_t value) {
    send(value, false);

    if (
        value == CMD_CLEAR_DISPLAY ||
        value == CMD_RETURN_HOME
    ) {
        delayMicroseconds(2000);
    }
}

void LCD16x2::send(uint8_t value, bool dataRegister) {
    // In 4-bit mode, the high nibble is transferred first.
    writeNibble(static_cast<uint8_t>(value >> 4), dataRegister);
    writeNibble(static_cast<uint8_t>(value & 0x0F), dataRegister);

    // Most ST7066U commands and data writes require 37 us.
    delayMicroseconds(50);
}

void LCD16x2::writeNibble(
    uint8_t nibble,
    bool dataRegister
) {
    uint8_t portValue = mapNibbleToPortB(nibble);

    if (dataRegister) {
        portValue |= PIN_RS;
    }

    // PIN_RW is intentionally never set: all operations are writes.
    portValue &= static_cast<uint8_t>(~PIN_RW);

    // Present stable data and control signals before raising E.
    LiF_SPI::MCP23S17::writePortB(portValue);

    // Pulse E. SPI transaction time itself already makes this pulse
    // substantially longer than the LCD's minimum pulse-width requirement.
    LiF_SPI::MCP23S17::writePortB(
        static_cast<uint8_t>(portValue | PIN_E)
    );
    delayMicroseconds(1);

    LiF_SPI::MCP23S17::writePortB(portValue);
    delayMicroseconds(1);
}

}  // namespace LiF_LCD
