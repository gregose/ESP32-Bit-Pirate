#pragma once

#include <cstdint>

namespace TBeamSupreme {
constexpr bool isExternalPin(int pin) {
    return pin == 2 || pin == 21 || pin == 38 || pin == 39 ||
           pin == 43 || pin == 44 || pin == 48;
}

constexpr bool isSdPins(uint8_t clk, uint8_t miso, uint8_t mosi, uint8_t cs) {
    return clk == 36 && miso == 37 && mosi == 35 && cs == 47;
}

constexpr bool isRadioPins(uint8_t clk, uint8_t miso, uint8_t mosi,
                           uint8_t cs, uint8_t rst, uint8_t busy, uint8_t dio1) {
    return clk == 12 && miso == 13 && mosi == 11 && cs == 10 &&
           rst == 5 && busy == 4 && dio1 == 1;
}

#ifdef DEVICE_TBEAM_SUPREME_S3
inline bool radioReady = false;
inline bool displayReady = false;
inline bool sdReady = false;
inline bool sdMounted = false;
inline bool usbStorageActive = false;
#endif
}
