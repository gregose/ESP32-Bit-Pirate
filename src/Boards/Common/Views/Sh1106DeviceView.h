#pragma once

#if defined(DEVICE_TBEAM_SUPREME_S3) || (defined(DEVICE_CUSTOM) && defined(CUSTOM_DISPLAY_DRIVER_SH1106_SW_I2C))
#include "Interfaces/IDeviceView.h"
#include "SoftwareI2cDisplayBus.h"
#include <Arduino.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <memory>
#include <string_view>

struct Sh1106Config {
    uint8_t sda = 255, scl = 255;
    uint8_t address = 0;
    bool powered = true;
    bool excludeQmc6310 = false;
    uint8_t rotation = 0;
};

class Sh1106DeviceView final : public IDeviceView {
public:
    explicit Sh1106DeviceView(const Sh1106Config& config)
        : bus(gpio, config.sda, config.scl), config(config) {}
    void setPowerAvailable(bool available) { config.powered = available; }
    void initialize() override;
    SPIClass& getSharedSpiInstance() override { return SPI; }
    void* getScreen() override { return ready ? &display : nullptr; }
    void logo() override;
    void welcome(TerminalTypeEnum& type, std::string& info) override;
    void show(PinoutConfig& config) override;
    void loading() override;
    void adapterMode(const std::string& name, const std::string& description,
                     const std::vector<std::string>& details) override;
    void clear() override;
    void drawLogicTrace(uint8_t pin, const std::vector<uint8_t>& buffer, uint8_t step) override;
    void drawAnalogicTrace(uint8_t pin, const std::vector<uint8_t>& buffer, uint8_t step) override;
    void drawWaterfall(const std::string& title, float start, float end, const char* unit,
                       int rowIndex, int rowCount, int level) override;
    void setRotation(uint8_t rotation) override;
    void setBrightness(uint8_t brightness) override;
    uint8_t getBrightness() override { return brightness; }
    void topBar(const std::string& title, bool submenu, bool searchBar) override;
    void horizontalSelection(const std::vector<std::string>& options, uint16_t selected,
                             const std::string& description1, const std::string& description2) override;

private:
    struct Gpio {
        void release(uint8_t pin) { pinMode(pin, INPUT_PULLUP); }
        void low(uint8_t pin) { digitalWrite(pin, LOW); pinMode(pin, OUTPUT); }
        bool read(uint8_t pin) { return digitalRead(pin) != LOW; }
        void pause() { delayMicroseconds(5); }
    } gpio;
    SoftwareI2cDisplayBus<Gpio> bus;
    Sh1106Config config;
    U8G2 display;
    // Incremental waterfall rows need a retained frame, allocated only on demand.
    std::unique_ptr<uint8_t[]> buffer;
    bool attempted = false, ready = false, transferOk = true;
    uint8_t brightness = 100;
    uint8_t address = 0;
    char title[26] = "ESP32 BIT PIRATE";
    static uint8_t transfer(u8x8_t* screen, uint8_t message, uint8_t count, void* data);
    static uint8_t gpioDelay(u8x8_t*, uint8_t message, uint8_t count, void*);
    void flush();
    static void copyText(std::string_view value, char (&output)[26], size_t columns = 25);
    void frame(std::string_view heading);
    void text(std::string_view value, int x, int row, size_t columns = 25);
    void lines(std::string_view heading, const std::vector<std::string>& values);
    void trace(uint8_t pin, const std::vector<uint8_t>& values, uint8_t step, bool analog);
};
#endif
