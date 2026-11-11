#pragma once

#include <cstdint>
#include "LoRaDefaults.h"

struct LoRaRadioProfile {
    float frequency = LoRaDefaults::frequency;
    uint16_t bandwidth = 125;
    uint8_t spreadingFactor = 9;
    uint8_t codingRate = 7;
    int8_t power = 14;
    float tcxoVoltage = LoRaDefaults::tcxoVoltage;
    uint16_t preambleLength = 8;
    uint16_t syncWord = 0x1424;
    bool crc = true;
    bool invertIq = false;
};
