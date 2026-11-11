#pragma once

#include <cstdint>

constexpr int HSPI = 1;

class SPIClass {
public:
    explicit SPIClass(int host = 0) : host(host) {}
    int host;
    unsigned beginCalls = 0, endCalls = 0;
    void begin(uint8_t, uint8_t, uint8_t, uint8_t) { ++beginCalls; }
    void end() { ++endCalls; }
};

inline SPIClass SPI;
