#pragma once

#include "../../Vendors/SD.h"
#include "SPI.h"

constexpr int FILE_READ = 0, FILE_WRITE = 1, FILE_APPEND = 2;

struct FakeSdCard {
    bool mountResult = true;
    unsigned beginCalls = 0, endCalls = 0;
    SPIClass* lastSpi = nullptr;
    bool begin(uint8_t, SPIClass& spi) {
        ++beginCalls;
        lastSpi = &spi;
        return mountResult;
    }
    void end() { ++endCalls; }
    File open(const char*, int = FILE_READ) { return File(); }
    bool exists(const char*) { return false; }
    bool remove(const char*) { return false; }
    bool mkdir(const char*) { return false; }
    bool rmdir(const char*) { return false; }
};

inline FakeSdCard SD;
