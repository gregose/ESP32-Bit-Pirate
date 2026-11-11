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

// Onboard GNSS (L76K or MAX-M10S) on a fixed UART; NMEA at 9600 8N1 by default.
constexpr uint8_t gnssRxPin = 9;
constexpr uint8_t gnssTxPin = 8;
constexpr uint8_t gnssWakePin = 7;
constexpr uint32_t gnssDefaultBaud = 9600;

constexpr bool isGnssUartPin(int pin) {
    return pin == gnssRxPin || pin == gnssTxPin;
}

#ifdef DEVICE_TBEAM_SUPREME_S3
inline bool gnssSelected = false;
inline bool gnssPowered = false;

// Switches the GNSS rail and wake line; false if power could not be confirmed.
bool setGnssEnabled(bool on);

inline bool radioReady = false;
inline bool displayReady = false;
inline bool sdReady = false;
inline bool sdMounted = false;
inline bool usbStorageActive = false;
#endif
}
