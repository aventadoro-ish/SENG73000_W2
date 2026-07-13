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

        // ST7066U command bases and option bits.
        constexpr uint8_t CMD_CLEAR_DISPLAY = 0x01;
        constexpr uint8_t CMD_RETURN_HOME = 0x02;

        constexpr uint8_t CMD_ENTRY_MODE = 0x04;
        constexpr uint8_t ENTRY_INCREMENT = 0x02;
        constexpr uint8_t ENTRY_SHIFT = 0x01;

        constexpr uint8_t CMD_DISPLAY_CONTROL = 0x08;
        constexpr uint8_t DISPLAY_ON = 0x04;
        constexpr uint8_t CURSOR_ON = 0x02;
        constexpr uint8_t BLINK_ON = 0x01;

        constexpr uint8_t CMD_CURSOR_SHIFT = 0x10;
        constexpr uint8_t SHIFT_DISPLAY = 0x08;
        constexpr uint8_t SHIFT_RIGHT = 0x04;

        constexpr uint8_t CMD_FUNCTION_SET = 0x20;
        constexpr uint8_t FUNCTION_2_LINE = 0x08;
        constexpr uint8_t FUNCTION_5X11 = 0x04;

        constexpr uint8_t CMD_SET_DDRAM = 0x80;

        constexpr uint8_t ROW_OFFSETS[] = {
            0x00,
            0x40
        };

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

    LCD12x2 lcd;

    bool LCD12x2::begin(
        uint8_t columns,
        uint8_t rows,
        Font font
    ) {
        initialized_ = false;
        column_ = 0;
        row_ = 0;

        if (!setGeometry(columns, rows)) {
            return false;
        }

        // The ST7066U allows the 5x11 font only in one-line mode.
        if (rows_ > 1 && font == Font::Dots5x11) {
            return false;
        }

        if (!LiF_SPI::MCP23S17::begin(
                LiF_SPI::LCD_EXPANDER_CHANNEL
            )) {
            return false;
        }

        // Keep DB4-DB7, E, R/W and RS low while the LCD powers up.
        // R/W remains low because this driver uses fixed delays rather than
        // reading the LCD busy flag.
        LiF_SPI::MCP23S17::writePortB(0x00);

        delay(50);

        // Enter the ST7066U four-bit initialization sequence.
        writeNibble(0x03, false);
        delayMicroseconds(50);

        uint8_t functionSet = CMD_FUNCTION_SET;

        // Four-bit mode is selected because the DL bit remains clear.
        if (rows_ > 1) {
            functionSet |= FUNCTION_2_LINE;
        }

        if (font == Font::Dots5x11) {
            functionSet |= FUNCTION_5X11;
        }

        command(functionSet);
        command(functionSet);

        displayEnabled_ = false;
        cursorEnabled_ = false;
        blinkEnabled_ = false;
        applyDisplayControl();

        clear();

        textDirection_ = TextDirection::LeftToRight;
        autoScrollEnabled_ = false;
        applyEntryMode();

        displayEnabled_ = true;
        applyDisplayControl();

        initialized_ = true;
        setCursor(0, 0);

        return true;
    }

    bool LCD12x2::setGeometry(
        uint8_t columns,
        uint8_t rows
    ) {
        if (
            columns == 0 ||
            columns > 40 ||
            rows == 0 ||
            rows > 2
        ) {
            return false;
        }

        columns_ = columns;
        rows_ = rows;

        if (column_ >= columns_) {
            column_ = columns_ - 1;
        }

        if (row_ >= rows_) {
            row_ = rows_ - 1;
        }

        return true;
    }

    uint8_t LCD12x2::columns() const {
        return columns_;
    }

    uint8_t LCD12x2::rows() const {
        return rows_;
    }

    void LCD12x2::clear() {
        command(CMD_CLEAR_DISPLAY);
        column_ = 0;
        row_ = 0;
    }

    void LCD12x2::home() {
        command(CMD_RETURN_HOME);
        column_ = 0;
        row_ = 0;
    }

    void LCD12x2::setCursor(
        uint8_t column,
        uint8_t row
    ) {
        if (row >= rows_) {
            row = rows_ - 1;
        }

        if (column >= columns_) {
            column = columns_ - 1;
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

    void LCD12x2::setDisplayControl(
        bool displayEnabled,
        bool cursorEnabled,
        bool blinkEnabled
    ) {
        displayEnabled_ = displayEnabled;
        cursorEnabled_ = cursorEnabled;
        blinkEnabled_ = blinkEnabled;
        applyDisplayControl();
    }

    void LCD12x2::setDisplayEnabled(bool enabled) {
        displayEnabled_ = enabled;
        applyDisplayControl();
    }

    void LCD12x2::setCursorEnabled(bool enabled) {
        cursorEnabled_ = enabled;
        applyDisplayControl();
    }

    void LCD12x2::setBlinkEnabled(bool enabled) {
        blinkEnabled_ = enabled;
        applyDisplayControl();
    }

    void LCD12x2::display() {
        setDisplayEnabled(true);
    }

    void LCD12x2::noDisplay() {
        setDisplayEnabled(false);
    }

    void LCD12x2::cursor() {
        setCursorEnabled(true);
    }

    void LCD12x2::noCursor() {
        setCursorEnabled(false);
    }

    void LCD12x2::blink() {
        setBlinkEnabled(true);
    }

    void LCD12x2::noBlink() {
        setBlinkEnabled(false);
    }

    void LCD12x2::setEntryMode(
        TextDirection direction,
        bool autoScrollEnabled
    ) {
        textDirection_ = direction;
        autoScrollEnabled_ = autoScrollEnabled;
        applyEntryMode();
    }

    void LCD12x2::leftToRight() {
        textDirection_ = TextDirection::LeftToRight;
        applyEntryMode();
    }

    void LCD12x2::rightToLeft() {
        textDirection_ = TextDirection::RightToLeft;
        applyEntryMode();
    }

    void LCD12x2::setAutoScroll(bool enabled) {
        autoScrollEnabled_ = enabled;
        applyEntryMode();
    }

    void LCD12x2::autoscroll() {
        setAutoScroll(true);
    }

    void LCD12x2::noAutoscroll() {
        setAutoScroll(false);
    }

    void LCD12x2::scrollDisplayLeft() {
        command(
            static_cast<uint8_t>(
                CMD_CURSOR_SHIFT |
                SHIFT_DISPLAY
            )
        );
    }

    void LCD12x2::scrollDisplayRight() {
        command(
            static_cast<uint8_t>(
                CMD_CURSOR_SHIFT |
                SHIFT_DISPLAY |
                SHIFT_RIGHT
            )
        );
    }

    void LCD12x2::moveCursorLeft() {
        command(CMD_CURSOR_SHIFT);

        if (column_ > 0) {
            --column_;
        }
    }

    void LCD12x2::moveCursorRight() {
        command(
            static_cast<uint8_t>(
                CMD_CURSOR_SHIFT |
                SHIFT_RIGHT
            )
        );

        if (column_ + 1 < columns_) {
            ++column_;
        }
    }

    size_t LCD12x2::write(uint8_t value) {
        if (!initialized_) {
            return 0;
        }

        if (value == '\r') {
            return 1;
        }

        if (value == '\n') {
            setCursor(
                0,
                static_cast<uint8_t>((row_ + 1) % rows_)
            );
            return 1;
        }

        // Software wrapping is useful for a stationary display. When hardware
        // autoscroll is active, the visible window moves and software wrapping
        // would fight the controller's display-shift behavior.
        if (!autoScrollEnabled_) {
            if (
                textDirection_ == TextDirection::LeftToRight &&
                column_ >= columns_
            ) {
                setCursor(
                    0,
                    static_cast<uint8_t>((row_ + 1) % rows_)
                );
            } else if (
                textDirection_ == TextDirection::RightToLeft &&
                column_ >= columns_
            ) {
                setCursor(
                    columns_ - 1,
                    static_cast<uint8_t>((row_ + 1) % rows_)
                );
            }
        }

        send(value, true);
        updateSoftwareCursorAfterWrite();

        return 1;
    }

    void LCD12x2::command(uint8_t value) {
        send(value, false);

        if (
            value == CMD_CLEAR_DISPLAY ||
            value == CMD_RETURN_HOME
        ) {
            delayMicroseconds(2000);
        }
    }

    void LCD12x2::send(
        uint8_t value,
        bool dataRegister
    ) {
        // Four-bit mode transfers the high nibble first.
        writeNibble(
            static_cast<uint8_t>(value >> 4),
            dataRegister
        );

        writeNibble(
            static_cast<uint8_t>(value & 0x0F),
            dataRegister
        );

        // Most ST7066U commands and data writes require about 37 us.
        delayMicroseconds(50);
    }

    void LCD12x2::writeNibble(
        uint8_t nibble,
        bool dataRegister
    ) {
        uint8_t portValue = mapNibbleToPortB(nibble);

        if (dataRegister) {
            portValue |= PIN_RS;
        }

        // R/W remains low: every transfer is a write.
        portValue &= static_cast<uint8_t>(~PIN_RW);

        // Present stable data/control before pulsing E.
        LiF_SPI::MCP23S17::writePortB(portValue);

        LiF_SPI::MCP23S17::writePortB(
            static_cast<uint8_t>(portValue | PIN_E)
        );
        delayMicroseconds(1);

        LiF_SPI::MCP23S17::writePortB(portValue);
        delayMicroseconds(1);
    }

    void LCD12x2::applyDisplayControl() {
        uint8_t value = CMD_DISPLAY_CONTROL;

        if (displayEnabled_) {
            value |= DISPLAY_ON;
        }

        if (cursorEnabled_) {
            value |= CURSOR_ON;
        }

        if (blinkEnabled_) {
            value |= BLINK_ON;
        }

        command(value);
    }

    void LCD12x2::applyEntryMode() {
        uint8_t value = CMD_ENTRY_MODE;

        if (textDirection_ == TextDirection::LeftToRight) {
            value |= ENTRY_INCREMENT;
        }

        if (autoScrollEnabled_) {
            value |= ENTRY_SHIFT;
        }

        command(value);
    }

    void LCD12x2::updateSoftwareCursorAfterWrite() {
        if (textDirection_ == TextDirection::LeftToRight) {
            if (column_ < 0xFF) {
                ++column_;
            }
        } else {
            if (column_ > 0) {
                --column_;
            } else {
                // Mark the logical position as outside the visible row so the
                // next non-autoscrolling write wraps to the opposite edge.
                column_ = columns_;
            }
        }
    }

    void demo_loop() {
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

                if (charCnt == 12) {
                    LiF_LCD::lcd.setCursor(0, 1);
                } else if (charCnt == 24) {
                    LiF_LCD::lcd.clear();
                    LiF_LCD::lcd.setCursor(0, 0);
                    charCnt = 0;
                }
            }
        }
    }

}  // namespace LiF_LCD
