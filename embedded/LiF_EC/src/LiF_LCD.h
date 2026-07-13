#pragma once

#include <Arduino.h>

namespace LiF_LCD {

/**
 * Driver for a 16x2 ST7066U-compatible character LCD connected to
 * MCP23S17 PORTB in 4-bit mode.
 *
 * PORTB mapping:
 *   GPB0 -> DB7
 *   GPB1 -> DB6
 *   GPB2 -> DB5
 *   GPB3 -> DB4
 *   GPB4 -> not connected
 *   GPB5 -> E
 *   GPB6 -> R/W
 *   GPB7 -> RS
 */
class LCD16x2 : public Print {
public:
    bool begin();

    void clear();
    void home();
    void setCursor(uint8_t column, uint8_t row);
    void setDisplayEnabled(bool enabled);

    size_t write(uint8_t value) override;
    using Print::write;

private:
    static constexpr uint8_t COLUMN_COUNT = 16;
    static constexpr uint8_t ROW_COUNT = 2;

    void command(uint8_t value);
    void send(uint8_t value, bool dataRegister);
    void writeNibble(uint8_t nibble, bool dataRegister);

    bool initialized_ = false;
    uint8_t column_ = 0;
    uint8_t row_ = 0;
};

extern LCD16x2 lcd;

}  // namespace LiF_LCD
