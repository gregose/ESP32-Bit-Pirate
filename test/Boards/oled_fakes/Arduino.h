#pragma once
#include <cstdint>
#include <deque>
#include <string>
#include <vector>
#include <stdexcept>

constexpr int LOW = 0, OUTPUT = 1, INPUT_PULLUP = 2;
inline std::deque<int> oledDataReads;
inline bool oledClockStuck = false;
inline std::vector<unsigned> oledPins;
inline void pinMode(uint8_t pin, int) { oledPins.push_back(pin); }
inline void digitalWrite(uint8_t pin, int) { oledPins.push_back(pin); }
inline int digitalRead(uint8_t pin) {
    oledPins.push_back(pin);
    if (pin == 18) return !oledClockStuck;
    if (oledDataReads.empty()) throw std::runtime_error("Unexpected OLED SDA read");
    const int value = oledDataReads.front();
    oledDataReads.pop_front();
    return value;
}
inline void delay(unsigned) {}
inline void delayMicroseconds(unsigned) {}
struct OledTestSerial {
    std::vector<std::string> messages;
    void println(const char* value) { messages.emplace_back(value); }
};
inline OledTestSerial Serial;
