#ifdef TBEAM_TEST_STANDALONE
#define DEVICE_TBEAM_SUPREME_S3
#define SDCARD_CS_PIN 47
#define SDCARD_CLK_PIN 36
#define SDCARD_MISO_PIN 37
#define SDCARD_MOSI_PIN 35
#define UART_RX_PIN 44
#define UART_TX_PIN 43
#define UART_BAUD 9600
#endif

#include <algorithm>
#include <cstdlib>
#include <vector>
#include <unity.h>
#include "../../src/Boards/TBeamSupreme/TBeamSupremeConfig.h"
#include "../../src/Boards/TBeamSupreme/TBeamSupremePower.h"
#include "../../src/Boards/Common/Views/SoftwareI2cDisplayBus.h"
#include "../../src/States/GlobalState.h"
#ifdef TBEAM_TEST_STANDALONE
#include "../../src/Transformers/ProfileTransformer.cpp"
struct TestSerial {
    unsigned errors = 0;
    void println(const char*) { ++errors; }
} Serial;
constexpr int OUTPUT = 1, HIGH = 1;
void delay(unsigned) {}
void pinMode(uint8_t, int) {}
void digitalWrite(uint8_t, int) {}
#include "../../src/Services/SdService.cpp"
#include "../../src/Transformers/ArgTransformer.cpp"
#include "../../src/Transformers/TerminalCommandTransformer.cpp"
#include "../../src/Transformers/LoRaTransformer.cpp"
#include "../../src/Managers/UserInputManager.cpp"
#include "../../src/Shells/HelpShell.cpp"
#include "../../src/Controllers/LoRaController.cpp"
#include "../Controllers/test_LoRaController.cpp"
namespace tbeam_gnss_fake {
bool enableResult = true, disableResult = true;
unsigned enableCalls = 0, disableCalls = 0;
}
bool TBeamSupreme::setGnssEnabled(bool on) {
    if (!on) {
        ++tbeam_gnss_fake::disableCalls;
        if (tbeam_gnss_fake::disableResult) gnssPowered = false;
        return tbeam_gnss_fake::disableResult;
    }
    ++tbeam_gnss_fake::enableCalls;
    if (tbeam_gnss_fake::enableResult) gnssPowered = true;
    else if (tbeam_gnss_fake::disableResult) gnssPowered = false;
    else gnssPowered = true;
    return tbeam_gnss_fake::enableResult;
}
#include "../../src/Controllers/UartController.cpp"
#include "../Controllers/test_UartController.cpp"
#endif

namespace tbeam_supreme_tests {
struct FakePmu {
    bool display = true, gps = true, header = true, radio = true, sd = true;
    bool offFails = false, radioVoltageFails = false, sdEnableFails = false;
    bool displayVoltageFails = false, displayEnableFails = false, displayReadbackFails = false;
    bool displayOffFails = false, displayVoltageReadbackFails = false;
    bool radioReadbackFails = false;
    bool gpsEnableFails = false;
    bool gpsDisableFails = false;
    unsigned gpsVoltage = 1800;
    unsigned radioEnableCalls = 0, sdEnableCalls = 0;
    unsigned radioVoltage = 1800, sdVoltage = 1800, displayVoltage = 1800;

    bool disableALDO1() { if (displayOffFails) return false; display = false; return true; }
    bool disableALDO4() {
        if (offFails || gpsDisableFails) return false;
        gps = false;
        return true;
    }
    bool disableBLDO2() { header = false; return true; }
    bool disableALDO3() { radio = false; return true; }
    bool disableBLDO1() { sd = false; return true; }
    bool isEnableALDO1() { return display && !displayReadbackFails; }
    bool isEnableALDO4() { return gps; }
    bool isEnableBLDO2() { return header; }
    bool isEnableALDO3() { return radio && !radioReadbackFails; }
    bool isEnableBLDO1() { return sd; }
    bool setALDO3Voltage(unsigned mv) {
        if (radioVoltageFails) return false;
        radioVoltage = mv;
        return true;
    }
    bool setBLDO1Voltage(unsigned mv) { sdVoltage = mv; return true; }
    bool setALDO1Voltage(unsigned mv) {
        if (displayVoltageFails) return false;
        displayVoltage = mv;
        return true;
    }
    unsigned getALDO1Voltage() { return displayVoltageReadbackFails ? 1800 : displayVoltage; }
    bool enableALDO1() { display = !displayEnableFails; return display; }
    unsigned getALDO3Voltage() { return radioVoltage; }
    unsigned getBLDO1Voltage() { return sdVoltage; }
    bool enableALDO3() { ++radioEnableCalls; radio = true; return true; }
    bool enableBLDO1() { ++sdEnableCalls; sd = !sdEnableFails; return sd; }
    bool setALDO4Voltage(unsigned mv) { gpsVoltage = mv; return true; }
    unsigned getALDO4Voltage() { return gpsVoltage; }
    bool enableALDO4() { gps = !gpsEnableFails; return gps; }
};

void test_only_exposed_pins_are_available() {
    const std::vector<int> expected = {2, 21, 38, 39, 43, 44, 48};
    GlobalState state;
    for (int pin = 0; pin <= 255; ++pin) {
        const bool available = std::find(expected.begin(), expected.end(), pin) != expected.end();
        TEST_ASSERT_EQUAL(available, TBeamSupreme::isExternalPin(pin));
#ifdef DEVICE_TBEAM_SUPREME_S3
        TEST_ASSERT_EQUAL(!available, state.isPinProtected(pin));
#endif
    }
    TEST_ASSERT_FALSE(TBeamSupreme::isExternalPin(-1));
}

void test_gnss_rail_is_switched_only_on_request() {
    FakePmu pmu;
    TBeamSupreme::configurePower(pmu);
    TEST_ASSERT_FALSE(pmu.gps);
    TEST_ASSERT_TRUE(TBeamSupreme::setGnssPower(pmu, true));
    TEST_ASSERT_TRUE(pmu.gps);
    TEST_ASSERT_EQUAL(3300, pmu.gpsVoltage);
    TEST_ASSERT_TRUE(TBeamSupreme::setGnssPower(pmu, false));
    TEST_ASSERT_FALSE(pmu.gps);
    pmu.gpsEnableFails = true;
    TEST_ASSERT_FALSE(TBeamSupreme::setGnssPower(pmu, true));
    TEST_ASSERT_FALSE(pmu.gps);
    pmu.gpsEnableFails = false;
    TEST_ASSERT_TRUE(TBeamSupreme::setGnssPower(pmu, true));
    pmu.gpsDisableFails = true;
    TEST_ASSERT_FALSE(TBeamSupreme::setGnssPower(pmu, false));
    TEST_ASSERT_TRUE(pmu.gps);
}

void test_gnss_uart_pins_stay_protected() {
    GlobalState state;
    for (int pin : {6, 7, 8, 9}) {
        TEST_ASSERT_FALSE(TBeamSupreme::isExternalPin(pin));
#ifdef DEVICE_TBEAM_SUPREME_S3
        TEST_ASSERT_TRUE(state.isPinProtected(pin));
#endif
    }
    TEST_ASSERT_TRUE(TBeamSupreme::isGnssUartPin(8));
    TEST_ASSERT_TRUE(TBeamSupreme::isGnssUartPin(9));
    TEST_ASSERT_FALSE(TBeamSupreme::isGnssUartPin(7));
    TEST_ASSERT_EQUAL(7, TBeamSupreme::gnssWakePin);
}

void test_fixed_onboard_wiring_is_not_generic() {
    TEST_ASSERT_TRUE(TBeamSupreme::isSdPins(36, 37, 35, 47));
    TEST_ASSERT_FALSE(TBeamSupreme::isSdPins(36, 35, 37, 47));
    TEST_ASSERT_TRUE(TBeamSupreme::isRadioPins(12, 13, 11, 10, 5, 4, 1));
    TEST_ASSERT_FALSE(TBeamSupreme::isRadioPins(12, 11, 13, 10, 5, 4, 1));
}

void test_board_profile_and_internal_sd_defaults() {
    GlobalState state;
    LoRaRadioProfile profile;
#ifdef DEVICE_TBEAM_SUPREME_S3
#ifdef LORA_DEFAULT_FREQUENCY
    constexpr float expectedFrequency = LORA_DEFAULT_FREQUENCY;
#else
    constexpr float expectedFrequency = 868.0f;
#endif
    TEST_ASSERT_FLOAT_WITHIN(0.001f, expectedFrequency, state.getLoRaFrequency());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.6f, state.getLoRaTcxoVoltage());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, expectedFrequency, profile.frequency);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.6f, profile.tcxoVoltage);
    TEST_ASSERT_TRUE(state.getHasInternalSdCard());
    TEST_ASSERT_TRUE(TBeamSupreme::isSdPins(state.getSdCardClkPin(), state.getSdCardMisoPin(),
                                          state.getSdCardMosiPin(), state.getSdCardCsPin()));
#else
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 868.0f, profile.frequency);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.8f, profile.tcxoVoltage);
#endif
    state.setLoRaFrequency(916.25f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 916.25f, state.getLoRaProfile().frequency);
}

void test_successful_power_setup_leaves_unused_rails_off() {
    FakePmu pmu;
    const auto status = TBeamSupreme::configurePower(pmu);
    TEST_ASSERT_TRUE(status.unusedRailsOff);
    TEST_ASSERT_TRUE(status.radio);
    TEST_ASSERT_TRUE(status.sd);
    TEST_ASSERT_TRUE(status.display);
    TEST_ASSERT_TRUE(pmu.display);
    TEST_ASSERT_FALSE(pmu.gps);
    TEST_ASSERT_FALSE(pmu.header);
    TEST_ASSERT_EQUAL(3300, pmu.radioVoltage);
    TEST_ASSERT_EQUAL(3300, pmu.sdVoltage);
    TEST_ASSERT_EQUAL(3300, pmu.displayVoltage);
}

void test_failed_unused_rail_shutdown_never_enables_consumers() {
    FakePmu pmu;
    pmu.offFails = true;
    const auto status = TBeamSupreme::configurePower(pmu);
    TEST_ASSERT_FALSE(status.unusedRailsOff);
    TEST_ASSERT_FALSE(status.radio);
    TEST_ASSERT_FALSE(status.sd);
    TEST_ASSERT_FALSE(status.display);
    TEST_ASSERT_EQUAL(0, pmu.radioEnableCalls);
    TEST_ASSERT_EQUAL(0, pmu.sdEnableCalls);
}

void test_failed_radio_voltage_keeps_radio_disabled_but_sd_available() {
    FakePmu pmu;
    pmu.radioVoltageFails = true;
    const auto status = TBeamSupreme::configurePower(pmu);
    TEST_ASSERT_FALSE(status.radio);
    TEST_ASSERT_FALSE(pmu.radio);
    TEST_ASSERT_EQUAL(0, pmu.radioEnableCalls);
    TEST_ASSERT_TRUE(status.sd);
}

void test_failed_sd_enable_does_not_disable_radio() {
    FakePmu pmu;
    pmu.sdEnableFails = true;
    const auto status = TBeamSupreme::configurePower(pmu);
    TEST_ASSERT_TRUE(status.radio);
    TEST_ASSERT_FALSE(status.sd);
}

void test_enable_readback_required_before_advertising_radio() {
    FakePmu pmu;
    pmu.radioReadbackFails = true;
    TEST_ASSERT_FALSE(TBeamSupreme::configurePower(pmu).radio);
    TEST_ASSERT_FALSE(pmu.radio);
}

void test_display_power_failure_is_independent_of_radio_and_sd() {
    for (unsigned failure = 0; failure < 5; ++failure) {
        FakePmu pmu;
        pmu.displayVoltageFails = failure == 0;
        pmu.displayEnableFails = failure == 1;
        pmu.displayReadbackFails = failure == 2;
        pmu.displayOffFails = failure == 3;
        pmu.displayVoltageReadbackFails = failure == 4;
        const auto status = TBeamSupreme::configurePower(pmu);
        TEST_ASSERT_FALSE(status.display);
        if (!pmu.displayOffFails) TEST_ASSERT_FALSE(pmu.display);
        TEST_ASSERT_TRUE(status.radio);
        TEST_ASSERT_TRUE(status.sd);
    }
}

struct FakeDisplayGpio {
    unsigned pauses = 0, sclReads = 0;
    bool stuckClock = false, stuckData = false, nack = false;
    std::vector<uint8_t> accessed;
    bool released[19] = {};
    void release(uint8_t pin) { accessed.push_back(pin); released[pin] = true; }
    void low(uint8_t pin) { accessed.push_back(pin); released[pin] = false; }
    bool read(uint8_t pin) {
        accessed.push_back(pin);
        if (pin == 18) { ++sclReads; return !stuckClock; }
        // SDA is idle-high on START, then low for each byte's ACK.
        return !stuckData && (sclReads == 1 || nack);
    }
    void pause() { ++pauses; }
};

void test_display_transport_is_bounded_and_owns_only_oled_pins() {
    FakeDisplayGpio gpio;
    SoftwareI2cDisplayBus<FakeDisplayGpio> bus(gpio, 17, 18);
    TEST_ASSERT_TRUE(bus.start(0x3C));
    TEST_ASSERT_TRUE(bus.write(0xAF));
    for (auto pin : gpio.accessed) TEST_ASSERT_TRUE(pin == 17 || pin == 18);
    gpio.stuckClock = true;
    const unsigned before = gpio.pauses;
    TEST_ASSERT_FALSE(bus.write(0));
    TEST_ASSERT_LESS_THAN_UINT(104, gpio.pauses - before);
    TEST_ASSERT_TRUE(gpio.released[17]);
    TEST_ASSERT_TRUE(gpio.released[18]);
}

void test_display_transport_reports_missing_device_and_stuck_data() {
    FakeDisplayGpio gpio;
    SoftwareI2cDisplayBus<FakeDisplayGpio> bus(gpio, 17, 18);
    gpio.nack = true;
    TEST_ASSERT_FALSE(bus.start(0x3D));
    gpio.nack = false;
    gpio.stuckData = true;
    gpio.sclReads = 0;
    TEST_ASSERT_FALSE(bus.start(0x3C));
    TEST_ASSERT_TRUE(gpio.released[17]);
    TEST_ASSERT_TRUE(gpio.released[18]);
}

#ifdef TBEAM_TEST_STANDALONE
void test_imported_profile_rejects_reserved_pins_before_mutation() {
    auto& state = GlobalState::getInstance();
    state.setSpiCSPin(2);
    std::string error;
    TEST_ASSERT_FALSE(ProfileTransformer::fromProfileText("spi.cs=21\ni2c.sda=42\n", error));
    TEST_ASSERT_EQUAL(2, state.getSpiCSPin());
    TEST_ASSERT_TRUE(error.find("i2c.sda") != std::string::npos);
    TEST_ASSERT_FALSE(ProfileTransformer::fromProfileText("sd.cs=2\n", error));
    TEST_ASSERT_EQUAL(47, state.getSdCardCsPin());
    TEST_ASSERT_TRUE(ProfileTransformer::fromProfileText("spi.cs=21\nsd.cs=47\n", error));
    TEST_ASSERT_EQUAL(21, state.getSpiCSPin());
}

void test_sd_requires_power_and_uses_separate_host() {
    SdService service;
    TBeamSupreme::sdReady = false;
    const unsigned before = SD.beginCalls;
    TEST_ASSERT_FALSE(service.configure(36, 37, 35, 47));
    TEST_ASSERT_TRUE(std::string(service.getLastError()).find("SD power") != std::string::npos);
    TEST_ASSERT_EQUAL(before, SD.beginCalls);
    TBeamSupreme::sdReady = true;
    TEST_ASSERT_TRUE(service.configure(36, 37, 35, 47));
    TEST_ASSERT_EQUAL_STRING("", service.getLastError());
    TEST_ASSERT_EQUAL(HSPI, SD.lastSpi->host);
    TEST_ASSERT_EQUAL(0, SPI.beginCalls);
    TEST_ASSERT_TRUE(TBeamSupreme::sdMounted);
    const unsigned mounted = SD.beginCalls;
    TEST_ASSERT_TRUE(service.configure(36, 37, 35, 47));
    TEST_ASSERT_EQUAL(mounted, SD.beginCalls);
    service.end();
    const unsigned ended = SD.endCalls;
    service.end();
    TEST_ASSERT_EQUAL(ended, SD.endCalls);
    TEST_ASSERT_FALSE(TBeamSupreme::sdMounted);
    TEST_ASSERT_EQUAL(0, SPI.endCalls);
}

void test_sd_remounts_external_bus_and_cleans_failed_mounts() {
    SdService service;
    TBeamSupreme::sdReady = true;
    TEST_ASSERT_TRUE(service.configure(36, 37, 35, 47));
    const unsigned before = SD.endCalls;
    TEST_ASSERT_TRUE(service.configure(38, 39, 48, 2));
    TEST_ASSERT_EQUAL(before + 1, SD.endCalls);
    TEST_ASSERT_EQUAL_PTR(&SPI, SD.lastSpi);
    service.end();
    SD.mountResult = false;
    TEST_ASSERT_FALSE(service.configure(36, 37, 35, 47));
    TEST_ASSERT_EQUAL_STRING("Mount failed. Check config and wiring and try again.", service.getLastError());
    TEST_ASSERT_FALSE(service.getSdState());
    TEST_ASSERT_FALSE(TBeamSupreme::sdMounted);
    SD.mountResult = true;
    TEST_ASSERT_TRUE(service.configure(36, 37, 35, 47));
    service.end();
}

void test_sd_rejects_reserved_pins_and_usb_storage_ownership() {
    SdService service;
    const unsigned before = SD.beginCalls;
    TEST_ASSERT_FALSE(service.configure(41, 42, 48, 2));
    TEST_ASSERT_TRUE(std::string(service.getLastError()).find("fixed") != std::string::npos);
    TEST_ASSERT_EQUAL(before, SD.beginCalls);
    TBeamSupreme::usbStorageActive = true;
    TEST_ASSERT_FALSE(service.configure(36, 37, 35, 47));
    TEST_ASSERT_TRUE(std::string(service.getLastError()).find("USB Stick") != std::string::npos);
    TEST_ASSERT_FALSE(service.configure(38, 39, 48, 2));
    TEST_ASSERT_EQUAL(before, SD.beginCalls);
    TBeamSupreme::usbStorageActive = false;
}

void test_radio_reset_uses_selected_band_and_reentry_preserves_runtime_frequency() {
    lora_controller_tests::LoRaControllerFixture fixture;
    TBeamSupreme::radioReady = true;
    fixture.input.queueLine("n");
    fixture.controller.handleCommand(TerminalCommand("config"));
    TEST_ASSERT_EQUAL(1, fixture.service.configurations.size());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, LoRaDefaults::frequency,
                            fixture.service.configurations.back().profile.frequency);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.6f,
                            fixture.service.configurations.back().profile.tcxoVoltage);
    fixture.controller.ensureReleased();
    GlobalState::getInstance().setLoRaFrequency(916.25f);
    fixture.input.queueLine("n");
    fixture.controller.ensureConfigured();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 916.25f,
                            fixture.service.configurations.back().profile.frequency);
    fixture.controller.ensureReleased();
    const auto configured = fixture.service.configurations.size();
    TBeamSupreme::radioReady = false;
    fixture.controller.ensureConfigured();
    TEST_ASSERT_EQUAL(configured, fixture.service.configurations.size());
    TEST_ASSERT_TRUE(fixture.view.contains("Onboard radio power unavailable."));
}

void test_uart_file_operations_route_to_internal_sd_on_this_board() {
    uart_controller_tests::UartControllerFixture fixture;
    fixture.terminalInput.queueLine("");
    fixture.terminalInput.queueLine("");
    fixture.terminalInput.queueLine("");
    fixture.controller.handleCommand(TerminalCommand("xmodem", "send", "/missing.bin"));
    TEST_ASSERT_EQUAL(1, fixture.sdService.configureCalls);
    TEST_ASSERT_TRUE(TBeamSupreme::isSdPins(fixture.sdService.lastClk, fixture.sdService.lastMiso,
                                          fixture.sdService.lastMosi, fixture.sdService.lastCs));
    TEST_ASSERT_EQUAL(1, fixture.sdService.endCalls);
}

void resetGnss() {
    TBeamSupreme::gnssSelected = false;
    TBeamSupreme::gnssPowered = false;
    tbeam_gnss_fake::enableResult = true;
    tbeam_gnss_fake::disableResult = true;
    tbeam_gnss_fake::enableCalls = 0;
    tbeam_gnss_fake::disableCalls = 0;
}

void configureExternalUart(uart_controller_tests::UartControllerFixture& fixture) {
    GlobalState::getInstance().setUartRxPin(44);
    GlobalState::getInstance().setUartTxPin(43);
    for (int i = 0; i < 7; ++i) fixture.terminalInput.queueLine("");
    fixture.controller.ensureConfigured();
}

void test_uart_onboard_gnss_uses_fixed_pins_and_default_baud() {
    resetGnss();
    uart_controller_tests::UartControllerFixture fixture;
    configureExternalUart(fixture);
    fixture.terminalInput.queueLine("");
    fixture.controller.handleCommand(TerminalCommand("config", "gnss"));
    TEST_ASSERT_TRUE(TBeamSupreme::gnssSelected);
    TEST_ASSERT_TRUE(TBeamSupreme::gnssPowered);
    const auto& cfg = fixture.uartService.configurations.back();
    TEST_ASSERT_EQUAL(9, cfg.rx);
    TEST_ASSERT_EQUAL(8, cfg.tx);
    TEST_ASSERT_EQUAL(9600, cfg.baud);
    TEST_ASSERT_FALSE(cfg.inverted);
    TEST_ASSERT_EQUAL(44, GlobalState::getInstance().getUartRxPin());
    TEST_ASSERT_EQUAL(43, GlobalState::getInstance().getUartTxPin());
    TEST_ASSERT_TRUE(fixture.view.contains("UART: using onboard GNSS (RX 9, TX 8, 8N1)."));

    fixture.controller.handleCommand(TerminalCommand("sniff"));
    TEST_ASSERT_EQUAL(1, fixture.uartSnifferService.textCalls);
    TEST_ASSERT_FALSE(fixture.view.contains("protected pin"));

    const auto configured = fixture.uartService.configurations.size();
    fixture.controller.handleCommand(TerminalCommand("swap"));
    TEST_ASSERT_TRUE(fixture.view.contains("UART Swap: Onboard GNSS pins are fixed."));
    TEST_ASSERT_EQUAL(44, GlobalState::getInstance().getUartRxPin());
    TEST_ASSERT_EQUAL(43, GlobalState::getInstance().getUartTxPin());
    TEST_ASSERT_EQUAL(configured, fixture.uartService.configurations.size());

    fixture.controller.handleCommand(TerminalCommand("emulator"));
    TEST_ASSERT_TRUE(fixture.view.contains("UART Emulator: Not available with onboard GNSS."));
    resetGnss();
}

void test_uart_onboard_gnss_powers_down_on_release_and_back_on_reentry() {
    resetGnss();
    uart_controller_tests::UartControllerFixture fixture;
    configureExternalUart(fixture);
    fixture.terminalInput.queueLine("38400");
    fixture.controller.handleCommand(TerminalCommand("config", "gnss"));
    TEST_ASSERT_EQUAL(38400, fixture.uartService.configurations.back().baud);

    const auto ends = fixture.uartService.endCalls;
    fixture.controller.ensureReleased();
    TEST_ASSERT_FALSE(TBeamSupreme::gnssPowered);
    TEST_ASSERT_TRUE(TBeamSupreme::gnssSelected);
    TEST_ASSERT_EQUAL(1, tbeam_gnss_fake::disableCalls);
    TEST_ASSERT_EQUAL(ends + 1, fixture.uartService.endCalls);

    fixture.controller.ensureConfigured();
    TEST_ASSERT_TRUE(TBeamSupreme::gnssPowered);
    TEST_ASSERT_EQUAL(2, tbeam_gnss_fake::enableCalls);
    TEST_ASSERT_EQUAL(9, fixture.uartService.configurations.back().rx);
    TEST_ASSERT_EQUAL(38400, fixture.uartService.configurations.back().baud);

    fixture.terminalInput.queueLine("");
    fixture.terminalInput.queueLine("");
    fixture.terminalInput.queueLine("");
    fixture.terminalInput.queueLine("");
    fixture.terminalInput.queueLine("");
    fixture.terminalInput.queueLine("");
    fixture.terminalInput.queueLine("");
    fixture.controller.handleCommand(TerminalCommand("config"));
    TEST_ASSERT_FALSE(TBeamSupreme::gnssSelected);
    TEST_ASSERT_FALSE(TBeamSupreme::gnssPowered);
    TEST_ASSERT_FALSE(TBeamSupreme::isGnssUartPin(fixture.uartService.configurations.back().rx));
    TEST_ASSERT_FALSE(TBeamSupreme::isGnssUartPin(fixture.uartService.configurations.back().tx));
    resetGnss();
}

void test_uart_onboard_gnss_power_failure_falls_back_to_external_pins() {
    resetGnss();
    tbeam_gnss_fake::enableResult = false;
    uart_controller_tests::UartControllerFixture fixture;
    GlobalState::getInstance().setUartRxPin(44);
    GlobalState::getInstance().setUartTxPin(43);
    fixture.controller.handleCommand(TerminalCommand("config", "gnss"));
    TEST_ASSERT_TRUE(fixture.view.contains("❌ UART onboard GNSS setup failed."));
    TEST_ASSERT_TRUE(fixture.view.contains("Onboard GNSS power unavailable."));
    TEST_ASSERT_FALSE(TBeamSupreme::gnssSelected);
    TEST_ASSERT_FALSE(TBeamSupreme::gnssPowered);
    TEST_ASSERT_EQUAL(44, fixture.uartService.configurations.back().rx);
    TEST_ASSERT_EQUAL(43, fixture.uartService.configurations.back().tx);
    resetGnss();
}

void test_failed_gnss_enable_and_power_off_keeps_gpio_reserved() {
    resetGnss();
    tbeam_gnss_fake::enableResult = false;
    tbeam_gnss_fake::disableResult = false;
    uart_controller_tests::UartControllerFixture fixture;
    GlobalState::getInstance().setUartRxPin(44);
    GlobalState::getInstance().setUartTxPin(43);
    fixture.controller.handleCommand(TerminalCommand("config", "gnss"));
    TEST_ASSERT_TRUE(TBeamSupreme::gnssPowered);
    TEST_ASSERT_FALSE(TBeamSupreme::gnssSelected);
    TEST_ASSERT_TRUE(fixture.view.contains("Onboard GNSS power unavailable."));
    TEST_ASSERT_TRUE(fixture.view.contains("GPIO 8/9 remain reserved."));
    TEST_ASSERT_EQUAL(0, fixture.uartService.configurations.size());
    TEST_ASSERT_EQUAL(44, GlobalState::getInstance().getUartRxPin());
    TEST_ASSERT_EQUAL(43, GlobalState::getInstance().getUartTxPin());
    resetGnss();
}

void test_generic_uart_prompt_rejects_gnss_pins() {
    resetGnss();
    uart_controller_tests::UartControllerFixture fixture;
    GlobalState::getInstance().setUartRxPin(44);
    GlobalState::getInstance().setUartTxPin(43);
    fixture.terminalInput.queueLine("9");
    fixture.terminalInput.queueLine("44");
    fixture.terminalInput.queueLine("8");
    fixture.terminalInput.queueLine("43");
    for (int i = 0; i < 6; ++i) fixture.terminalInput.queueLine("");
    fixture.controller.handleCommand(TerminalCommand("config"));
    TEST_ASSERT_TRUE(fixture.view.contains("This GPIO is reserved/protected and cannot be used."));
    TEST_ASSERT_EQUAL(44, fixture.uartService.configurations.back().rx);
    TEST_ASSERT_EQUAL(43, fixture.uartService.configurations.back().tx);
    TEST_ASSERT_EQUAL(0, tbeam_gnss_fake::enableCalls);
}

void test_failed_gnss_power_off_is_reported_and_remains_reserved() {
    resetGnss();
    uart_controller_tests::UartControllerFixture fixture;
    configureExternalUart(fixture);
    fixture.terminalInput.queueLine("");
    fixture.controller.handleCommand(TerminalCommand("config", "gnss"));
    TEST_ASSERT_TRUE(TBeamSupreme::gnssPowered);

    tbeam_gnss_fake::disableResult = false;
    fixture.controller.ensureReleased();
    TEST_ASSERT_TRUE(TBeamSupreme::gnssPowered);
    TEST_ASSERT_TRUE(TBeamSupreme::gnssSelected);
    TEST_ASSERT_TRUE(fixture.view.contains("Onboard GNSS power could not be disabled. GPIO 8/9 remain reserved."));

    const auto enabled = tbeam_gnss_fake::enableCalls;
    fixture.controller.ensureConfigured();
    TEST_ASSERT_EQUAL(enabled, tbeam_gnss_fake::enableCalls);
    TEST_ASSERT_EQUAL(9, fixture.uartService.configurations.back().rx);
    TEST_ASSERT_EQUAL(8, fixture.uartService.configurations.back().tx);
    resetGnss();
}

void test_generic_uart_config_keeps_external_prompt_sequence() {
    resetGnss();
    uart_controller_tests::UartControllerFixture fixture;
    GlobalState::getInstance().setUartRxPin(44);
    GlobalState::getInstance().setUartTxPin(43);
    for (int i = 0; i < 7; ++i) fixture.terminalInput.queueLine("");
    fixture.controller.handleCommand(TerminalCommand("config"));
    TEST_ASSERT_EQUAL(44, fixture.uartService.configurations.back().rx);
    TEST_ASSERT_EQUAL(43, fixture.uartService.configurations.back().tx);
    TEST_ASSERT_EQUAL(9600, fixture.uartService.configurations.back().baud);
    TEST_ASSERT_FALSE(fixture.view.contains("Use onboard GNSS?"));
    TEST_ASSERT_EQUAL(0, tbeam_gnss_fake::enableCalls);

    const std::vector<std::string> prompts = {
        "RX GPIO number",
        "TX GPIO number",
        "Baud rate",
        "Data bits (5-8)",
        "Parity (N/E/O)",
        "Stop bits (1 or 2)",
        "Inverted?"
    };
    size_t cursor = 0;
    for (const auto& prompt : prompts) {
        const auto position = fixture.view.output.find(prompt, cursor);
        TEST_ASSERT_NOT_EQUAL(std::string::npos, position);
        cursor = position + prompt.size();
    }
}
#endif
}

void runTBeamSupremeTests() {
    using namespace tbeam_supreme_tests;
    RUN_TEST(test_only_exposed_pins_are_available);
    RUN_TEST(test_fixed_onboard_wiring_is_not_generic);
    RUN_TEST(test_gnss_rail_is_switched_only_on_request);
    RUN_TEST(test_gnss_uart_pins_stay_protected);
    RUN_TEST(test_board_profile_and_internal_sd_defaults);
    RUN_TEST(test_successful_power_setup_leaves_unused_rails_off);
    RUN_TEST(test_failed_unused_rail_shutdown_never_enables_consumers);
    RUN_TEST(test_failed_radio_voltage_keeps_radio_disabled_but_sd_available);
    RUN_TEST(test_failed_sd_enable_does_not_disable_radio);
    RUN_TEST(test_enable_readback_required_before_advertising_radio);
    RUN_TEST(test_display_power_failure_is_independent_of_radio_and_sd);
    RUN_TEST(test_display_transport_is_bounded_and_owns_only_oled_pins);
    RUN_TEST(test_display_transport_reports_missing_device_and_stuck_data);
#ifdef TBEAM_TEST_STANDALONE
    RUN_TEST(test_imported_profile_rejects_reserved_pins_before_mutation);
    RUN_TEST(test_sd_requires_power_and_uses_separate_host);
    RUN_TEST(test_sd_remounts_external_bus_and_cleans_failed_mounts);
    RUN_TEST(test_sd_rejects_reserved_pins_and_usb_storage_ownership);
    RUN_TEST(test_radio_reset_uses_selected_band_and_reentry_preserves_runtime_frequency);
    RUN_TEST(test_uart_file_operations_route_to_internal_sd_on_this_board);
    RUN_TEST(test_uart_onboard_gnss_uses_fixed_pins_and_default_baud);
    RUN_TEST(test_uart_onboard_gnss_powers_down_on_release_and_back_on_reentry);
    RUN_TEST(test_uart_onboard_gnss_power_failure_falls_back_to_external_pins);
    RUN_TEST(test_failed_gnss_enable_and_power_off_keeps_gpio_reserved);
    RUN_TEST(test_generic_uart_prompt_rejects_gnss_pins);
    RUN_TEST(test_failed_gnss_power_off_is_reported_and_remains_reserved);
    RUN_TEST(test_generic_uart_config_keeps_external_prompt_sequence);
#endif
}

#ifdef TBEAM_TEST_STANDALONE
void setUp() {}
void tearDown() {}
int main(int, char**) {
    UNITY_BEGIN();
    runTBeamSupremeTests();
    return UNITY_END();
}
#endif
