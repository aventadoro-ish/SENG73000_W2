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

constexpr int LimSw_1 = PC5;
constexpr int LimSw_2 = PC6;


#else
#error "Please, define an embedded platform to be used.Either ESP32_ENV or STM32_ENV flags"
#endif
