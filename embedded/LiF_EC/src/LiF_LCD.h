#pragma once

/*
 * Library author: KMuthumala 
 * Source: https://github.com/KMuthumala/Interface-TOF-sensor-with-STM32
 */

#include <Arduino.h>

namespace LiF_LCD {

    enum class Font : uint8_t {
        Dots5x8,
        Dots5x11
    };

    enum class TextDirection : uint8_t {
        LeftToRight,
        RightToLeft
    };


    /**
     * Driver for a 12x2 ST7066U-compatible character LCD connected to
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
    class LCD12x2 : public Print {
    public:
        /**
         * @brief Initialize the display.
         *
         * @param columns Physical visible columns, 1 to 40.
         * @param rows Physical visible rows, either 1 or 2.
         * @param font 5x8 or 5x11 font. The 5x11 font is valid only in
         *             one-line mode.
         *
         * For CFAH1202A-YYH-JT, use begin(12, 2).
         */
        bool begin(
            uint8_t columns = 12,
            uint8_t rows = 2,
            Font font = Font::Dots5x8
        );
        /**
         * Change the driver's software geometry.
         *
         * This does not change the physical LCD glass or write a Function Set
         * command. It changes cursor clamping and software wrapping only.
         */
        bool setGeometry(uint8_t columns, uint8_t rows);

        uint8_t columns() const;
        uint8_t rows() const;

        void clear();
        void home();
        void setCursor(uint8_t column, uint8_t row);

        // Display, cursor, and blinking control.
        void setDisplayControl(
            bool displayEnabled,
            bool cursorEnabled,
            bool blinkEnabled
        );

        void setDisplayEnabled(bool enabled);
        void setCursorEnabled(bool enabled);
        void setBlinkEnabled(bool enabled);

        void display();
        void noDisplay();
        void cursor();
        void noCursor();
        void blink();
        void noBlink();

        // Entry-mode control.
        void setEntryMode(
            TextDirection direction,
            bool autoScrollEnabled
        );

        void leftToRight();
        void rightToLeft();

        /**
         * When enabled, each character write also shifts the complete display.
         *
         * Left-to-right entry shifts the display left.
         * Right-to-left entry shifts the display right.
         */
        void setAutoScroll(bool enabled);
        void autoscroll();
        void noAutoscroll();

        // One-shot cursor/display shift commands.
        void scrollDisplayLeft();
        void scrollDisplayRight();
        void moveCursorLeft();
        void moveCursorRight();

        /**
         * Arduino Print byte sink.
         *
         * Return value is 1 when the byte is accepted and 0 when the LCD has
         * not been initialized.
         */
        size_t write(uint8_t value) override;

        // Keep the other Print::write overloads visible, including
        // write(const uint8_t *buffer, size_t size).
        using Print::write;

    private:
        void command(uint8_t value);
        void send(uint8_t value, bool dataRegister);
        void writeNibble(uint8_t nibble, bool dataRegister);

        void applyDisplayControl();
        void applyEntryMode();
        void updateSoftwareCursorAfterWrite();

        bool initialized_ = false;

        uint8_t columns_ = 12;
        uint8_t rows_ = 2;
        uint8_t column_ = 0;
        uint8_t row_ = 0;

        bool displayEnabled_ = true;
        bool cursorEnabled_ = false;
        bool blinkEnabled_ = false;

        TextDirection textDirection_ = TextDirection::LeftToRight;
        bool autoScrollEnabled_ = false;
    };

    extern LCD12x2 lcd;
    void demo_loop();

}  // namespace LiF_LCD
