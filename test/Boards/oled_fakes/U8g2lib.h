#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>

constexpr uint8_t U8X8_MSG_DELAY_MILLI = 1, U8X8_MSG_BYTE_START_TRANSFER = 2,
    U8X8_MSG_BYTE_SEND = 3, U8X8_MSG_BYTE_END_TRANSFER = 4;
inline const void* U8G2_R0 = nullptr;
inline const void* U8G2_R2 = reinterpret_cast<void*>(2);
inline const uint8_t u8g2_font_5x7_tf[] = {0};
struct u8x8_t {
    void* user_ptr = nullptr;
    uint8_t (*callback)(u8x8_t*, uint8_t, uint8_t, void*) = nullptr;
};
struct u8g2_t { u8x8_t screen; const void* rotation = U8G2_R0; };
inline void u8g2_SetBufferPtr(u8g2_t*, uint8_t*) {}
inline void* u8x8_GetUserPtr(u8x8_t* s) { return s->user_ptr; }
inline void u8x8_SetUserPtr(u8x8_t* s, void* p) { s->user_ptr = p; }
inline unsigned oledSetupCalls = 0;
inline void u8g2_Setup_sh1106_i2c_128x64_noname_f(u8g2_t* d, const void* rotation,
        uint8_t (*callback)(u8x8_t*, uint8_t, uint8_t, void*),
        uint8_t (*)(u8x8_t*, uint8_t, uint8_t, void*)) {
    ++oledSetupCalls;
    d->screen.callback = callback;
    d->rotation = rotation;
}
class U8G2 {
    u8g2_t data;
    void command() {
        auto* s = &data.screen;
        uint8_t byte = 0;
        if (!s->callback(s, U8X8_MSG_BYTE_START_TRANSFER, 0, nullptr)) return;
        if (!s->callback(s, U8X8_MSG_BYTE_SEND, 1, &byte)) return;
        s->callback(s, U8X8_MSG_BYTE_END_TRANSFER, 0, nullptr);
    }
public:
    std::vector<std::string> labels;
    unsigned frames = 0;
    uint8_t address = 0, contrast = 0, powerSave = 0;
    u8g2_t* getU8g2() { return &data; }
    u8x8_t* getU8x8() { return &data.screen; }
    void initDisplay() { command(); }
    void setPowerSave(uint8_t value) { powerSave = value; command(); }
    void setContrast(uint8_t value) { contrast = value; command(); }
    void setFont(const uint8_t*) {}
    void setI2CAddress(uint8_t value) { address = value; }
    void clearBuffer() { labels.clear(); }
    void sendBuffer() { ++frames; command(); }
    void setDisplayRotation(const void* rotation) { data.rotation = rotation; }
    void drawStr(int x, int y, const char* text) {
        if (x < 0 || y < 0 || x + std::string(text).size() * 5 > 128 || y >= 64)
            throw std::runtime_error("OLED label outside 128x64");
        labels.emplace_back(text);
    }
    void drawHLine(int x, int y, int width) {
        if (x < 0 || y < 0 || y >= 64 || x + width > 128)
            throw std::runtime_error("OLED line outside 128x64");
    }
    void drawLine(int x, int y, int x2, int y2) {
        if (x < 0 || x2 >= 128 || y < 0 || y2 < 0 || y >= 64 || y2 >= 64)
            throw std::runtime_error("OLED trace outside 128x64");
    }
    void setDrawColor(uint8_t) {}
};
