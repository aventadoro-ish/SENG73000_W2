#pragma once
#include <Arduino.h>

#if defined(ESP32_ENV)
constexpr int IN1 = 23;
constexpr int IN2 = 19;
constexpr int IN3 = 22;
constexpr int IN4 = 18;

#elif defined(STM32_ENV)
constexpr int IN1 = PC1; // A1
constexpr int IN2 = PC0; // A2
constexpr int IN3 = PC2; // B1
constexpr int IN4 = PC3; // B2

constexpr int CAN_Tx = PB9;
constexpr int CAN_Rx = PB8;
constexpr int CAN_stb = PC8;

constexpr int LimSw_1 = PC6;
constexpr int LimSw_2 = PC5;

constexpr int I2C2_SDA = PA10;
constexpr int I2C2_SCL = PA9;

constexpr int SPI_MISO = PB4;
constexpr int SPI_MOSI = PB5;
constexpr int SPI_SCK  = PB3;

// 74x139 address and active-low enable pins.
constexpr uint32_t PIN_CSA0 = PB0;
constexpr uint32_t PIN_CSA1 = PC13;
constexpr uint32_t PIN_CSEN = PA4;

#else
#error "Please, define an embedded platform to be used.Either ESP32_ENV or STM32_ENV flags"
#endif
