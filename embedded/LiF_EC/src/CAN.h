#pragma once

#include <Arduino.h>
#include "pin_definition.h"

#if defined(STM32_ENV)
#include "STM32_CAN.h"

#else
#error "Please, define an embedded platform to be used.Either ESP32_ENV or STM32_ENV flags"
#endif

namespace LiF_CAN {
    // CAN message parameters
    constexpr int TxID = 0x101;
    constexpr int DLC = 1;
    constexpr int Floor1 = 0x05;
    constexpr int Floor2 = 0x06;
    constexpr int Floor3 = 0x07;

    // Filters
    constexpr int MASK = 0x07FF;
    constexpr int FILTER_SC = 0x0100;
    constexpr int FILTER_EC = 0x0101;
    constexpr int FILTER_CC = 0x0200;
    constexpr int FILTER_F1 = 0x0201;
    constexpr int FILTER_F2 = 0x0202;
    constexpr int FILTER_F3 = 0x0203;


    extern STM32_CAN bus;

    void setup();

    bool transmit(uint8_t floorByte);
    bool receive(uint32_t &rxId, uint8_t &rxLen, uint8_t rxData[8]);
    

}