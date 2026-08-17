#include "seven_seg.h"



void mapDigit1(uint16_t &output, uint8_t pattern);
void mapDigit2(uint16_t &output, uint8_t pattern);




// Logical segment positions used by the lookup table
constexpr uint8_t SEG_A = (1U << 0);
constexpr uint8_t SEG_B = (1U << 1);
constexpr uint8_t SEG_C = (1U << 2);
constexpr uint8_t SEG_D = (1U << 3);
constexpr uint8_t SEG_E = (1U << 4);
constexpr uint8_t SEG_F = (1U << 5);
constexpr uint8_t SEG_G = (1U << 6);

// Segment patterns for digits 0-9
const uint8_t digitPatterns[16] = {
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,         // 0
    SEG_B |         SEG_C,                                 // 1
    SEG_A | SEG_B |         SEG_D | SEG_E |         SEG_G, // 2
    SEG_A | SEG_B | SEG_C | SEG_D |                 SEG_G, // 3
    SEG_B |         SEG_C |                 SEG_F | SEG_G, // 4
    SEG_A |         SEG_C | SEG_D |         SEG_F | SEG_G, // 5
    SEG_A |         SEG_C | SEG_D | SEG_E | SEG_F | SEG_G, // 6
    SEG_A | SEG_B | SEG_C,                                 // 7
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G, // 8
    SEG_A | SEG_B | SEG_C | SEG_D |         SEG_F | SEG_G, // 9
    SEG_A | SEG_B | SEG_C |         SEG_E | SEG_F | SEG_G, // A 
                    SEG_C | SEG_D | SEG_E | SEG_F | SEG_G, // b
                            SEG_D | SEG_E |         SEG_G, // c
            SEG_B | SEG_C | SEG_D | SEG_E |         SEG_G, // d
    SEG_A |                 SEG_D | SEG_E | SEG_F | SEG_G, // E
    SEG_A                         | SEG_E | SEG_F | SEG_G  // F
};

void mapDigit1(uint16_t &output, uint8_t pattern) {
    if (pattern & SEG_A) output |= (1U << 13); // Port B pin 5
    if (pattern & SEG_B) output |= (1U << 12); // Port B pin 4
    if (pattern & SEG_C) output |= (1U << 2);  // Port A pin 2
    if (pattern & SEG_D) output |= (1U << 1);  // Port A pin 1
    if (pattern & SEG_E) output |= (1U << 0);  // Port A pin 0
    if (pattern & SEG_F) output |= (1U << 15); // Port B pin 7
    if (pattern & SEG_G) output |= (1U << 14); // Port B pin 6
}

void mapDigit2(uint16_t &output, uint8_t pattern) {
    if (pattern & SEG_A) output |= (1U << 10); // Port B pin 2
    if (pattern & SEG_B) output |= (1U << 9);  // Port B pin 1
    if (pattern & SEG_C) output |= (1U << 7);  // Port A pin 7
    if (pattern & SEG_D) output |= (1U << 5);  // Port A pin 5
    if (pattern & SEG_E) output |= (1U << 4);  // Port A pin 4
    if (pattern & SEG_F) output |= (1U << 11); // Port B pin 3
    if (pattern & SEG_G) output |= (1U << 6);  // Port A pin 6
}

uint16_t mapNumber(uint8_t number, bool digit1DP, bool digit2DP) {
    // Keep only the last two digits if number is greater than 99
    number %= 100;

    uint8_t digit1 = number / 10;
    uint8_t digit2 = number % 10;

    uint16_t output = 0;

    mapDigit1(output, digitPatterns[digit1]);
    mapDigit2(output, digitPatterns[digit2]);

    if (digit1DP) output |= (1U << 3); // Port A pin 3
    if (digit2DP) output |= (1U << 8); // Port B pin 0

    return output;
}

uint16_t mapNumberHex(uint8_t number) {
    uint8_t digit1 = number & 0x0f;
    uint8_t digit2 = (number & 0xf0) >> 8;

    uint16_t output = 0;
    mapDigit1(output, digitPatterns[digit1]);
    mapDigit2(output, digitPatterns[digit2]);

    return output;
}
