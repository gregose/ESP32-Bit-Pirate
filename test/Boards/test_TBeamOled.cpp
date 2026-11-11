#ifdef TBEAM_OLED_TEST
#define DEVICE_TBEAM_SUPREME_S3
#define U8X8_WITH_USER_PTR
#define U8G2_USE_DYNAMIC_ALLOC
#include <unity.h>
#include "../../src/Boards/TBeamSupreme/TBeamSupremeConfig.h"
#include "../../src/Boards/Common/Views/Sh1106DeviceView.cpp"

namespace {
void queue(std::initializer_list<int> values) {
    oledDataReads.insert(oledDataReads.end(), values.begin(), values.end());
}

void candidate(bool magnetometer) {
    queue({1, 0, 1}); // Probe START, ACK, STOP.
    queue({1, 0, 0, 1, 1, 0}); // Register write, STOP, read address.
    for (unsigned bit = 0; bit < 8; ++bit) queue({magnetometer && bit == 0 ? 1 : 0});
    queue({1}); // Read STOP.
}

void frames(unsigned count = 20) {
    for (unsigned i = 0; i < count; ++i) queue({1, 0, 0, 1});
}

bool has(const U8G2& display, const std::string& value) {
    return std::find(display.labels.begin(), display.labels.end(), value) != display.labels.end();
}

void test_missing_power_keeps_serial_and_spi_independent() {
    TBeamSupreme::displayReady = false;
    Sh1106DeviceView view(Sh1106Config{17, 18, 0, false});
    view.initialize();
    view.initialize();
    view.loading();
    TEST_ASSERT_NULL(view.getScreen());
    TEST_ASSERT_EQUAL_PTR(&SPI, &view.getSharedSpiInstance());
    TEST_ASSERT_TRUE(oledPins.empty());
    TEST_ASSERT_EQUAL(1, Serial.messages.size());
}

void test_address_selection_excludes_magnetometer_and_init_is_idempotent() {
    candidate(true);
    candidate(false);
    frames();
    Sh1106DeviceView view(Sh1106Config{17, 18, 0, true, true, 2});
    view.initialize();
    const auto reads = oledDataReads.size();
    view.initialize();
    TEST_ASSERT_EQUAL(reads, oledDataReads.size());
    auto* screen = static_cast<U8G2*>(view.getScreen());
    TEST_ASSERT_NOT_NULL(screen);
    TEST_ASSERT_EQUAL(0x3D << 1, screen->address);
    TEST_ASSERT_EQUAL_PTR(U8G2_R2, screen->getU8g2()->rotation);
    TEST_ASSERT_EQUAL(1, oledSetupCalls);
    for (auto pin : oledPins) TEST_ASSERT_TRUE(pin == 17 || pin == 18);
}

void test_ambiguous_or_missing_addresses_do_not_claim_display() {
    candidate(false);
    candidate(false);
    Sh1106DeviceView ambiguous(Sh1106Config{17, 18, 0, true, true});
    ambiguous.initialize();
    TEST_ASSERT_NULL(ambiguous.getScreen());
    TEST_ASSERT_EQUAL(0, oledSetupCalls);
    queue({1, 1, 1, 1, 1, 1});
    Sh1106DeviceView absent(Sh1106Config{17, 18, 0, true, true});
    absent.initialize();
    TEST_ASSERT_NULL(absent.getScreen());
}

void test_write_only_display_candidate_is_not_mistaken_for_failed_bus() {
    queue({1, 0, 1}); // Probe 0x3C.
    queue({1, 0, 0, 1, 1, 1, 1}); // Register write ACK, read address NACK, STOP.
    queue({1, 0, 1}); // Confirm write address still responds.
    queue({1, 1, 1}); // No 0x3D.
    frames();
    Sh1106DeviceView view(Sh1106Config{17, 18, 0, true, true});
    view.initialize();
    auto* screen = static_cast<U8G2*>(view.getScreen());
    TEST_ASSERT_NOT_NULL(screen);
    TEST_ASSERT_EQUAL(0x3C << 1, screen->address);
}

void test_generic_explicit_address_does_not_read_board_sensor_ids() {
    queue({1, 0, 1}); // Only a write probe, no sensor-register reads.
    frames();
    Sh1106DeviceView view(Sh1106Config{17, 18, 0x3D});
    view.initialize();
    auto* screen = static_cast<U8G2*>(view.getScreen());
    TEST_ASSERT_NOT_NULL(screen);
    TEST_ASSERT_EQUAL(0x3D << 1, screen->address);
    TEST_ASSERT_EQUAL_PTR(U8G2_R0, screen->getU8g2()->rotation);
    view.setRotation(2);
    TEST_ASSERT_EQUAL_PTR(U8G2_R2, screen->getU8g2()->rotation);
    view.setRotation(0);
    TEST_ASSERT_EQUAL_PTR(U8G2_R0, screen->getU8g2()->rotation);
    view.adapterMode(std::string(100000, 'X'), "\nOK\r", {std::string(100000, 'Y')});
    TEST_ASSERT_TRUE(has(*screen, std::string(24, 'X') + "~"));
    TEST_ASSERT_TRUE(has(*screen, std::string(11, 'Y') + "~"));
    TEST_ASSERT_TRUE(has(*screen, "OK"));
    view.topBar(std::string(100000, 'Z'), false, false);
    view.horizontalSelection({"SERIAL"}, 0, "Select terminal", "");
    TEST_ASSERT_TRUE(has(*screen, std::string(24, 'Z') + "~"));
}

void test_compact_existing_ux_fits_and_transport_failure_disables_only_display() {
    candidate(false);
    queue({1, 1, 1}); // No second candidate.
    frames(50);
    Sh1106DeviceView view(Sh1106Config{17, 18, 0, true, true});
    view.initialize();
    auto* screen = static_cast<U8G2*>(view.getScreen());
    TEST_ASSERT_NOT_NULL(screen);
    view.topBar("ESP32 BIT PIRATE", false, false);
    view.horizontalSelection({"WIFI CLIENT", "WIFI AP", "SERIAL"}, 2,
        "Terminal auto-select", "Short press: CONNECT  Long press: HOTSPOT");
    TEST_ASSERT_TRUE(has(*screen, "Short press: CONNECT"));
    TEST_ASSERT_TRUE(has(*screen, "No input: Serial (3s)"));
    TerminalTypeEnum type = TerminalTypeEnum::SerialPort;
    std::string baud = "115200";
    view.welcome(type, baud);
    TEST_ASSERT_TRUE(has(*screen, "Baudrate: 115200"));
    PinoutConfig config;
    config.setMode("LORA");
    config.setMappings({"SCK GPIO 12", "MISO GPIO 13", "MOSI GPIO 11", "CS GPIO 10"});
    view.show(config);
    TEST_ASSERT_TRUE(has(*screen, "LORA"));
    TEST_ASSERT_TRUE(has(*screen, "CS GPIO 10"));
    view.adapterMode(std::string(80, 'X'), "description", std::vector<std::string>(8, std::string(80, 'Y')));
    view.drawLogicTrace(2, {0, 1, 0, 1}, 0);
    view.drawAnalogicTrace(2, {0, 255, 0, 128}, 40);
    const auto before = screen->frames;
    view.drawWaterfall("LoRa", 914, 916, "MHz", 0, 100, -1);
    view.drawWaterfall("LoRa", 914, 916, "MHz", 99, 100, 200);
    TEST_ASSERT_EQUAL(before + 1, screen->frames);
    view.setBrightness(255);
    TEST_ASSERT_EQUAL(100, view.getBrightness());
    view.setBrightness(0);
    TEST_ASSERT_EQUAL(1, screen->powerSave);
    oledClockStuck = true;
    view.clear();
    TEST_ASSERT_NULL(view.getScreen());
    TEST_ASSERT_TRUE(TBeamSupreme::radioReady);
    TEST_ASSERT_TRUE(TBeamSupreme::sdReady);
    const auto accesses = oledPins.size();
    view.clear();
    TEST_ASSERT_EQUAL(accesses, oledPins.size());
}
}

void setUp() {
    oledDataReads.clear();
    oledPins.clear();
    oledClockStuck = false;
    oledSetupCalls = 0;
    Serial.messages.clear();
    TBeamSupreme::displayReady = TBeamSupreme::radioReady = TBeamSupreme::sdReady = true;
}
void tearDown() {}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_missing_power_keeps_serial_and_spi_independent);
    RUN_TEST(test_address_selection_excludes_magnetometer_and_init_is_idempotent);
    RUN_TEST(test_ambiguous_or_missing_addresses_do_not_claim_display);
    RUN_TEST(test_write_only_display_candidate_is_not_mistaken_for_failed_bus);
    RUN_TEST(test_generic_explicit_address_does_not_read_board_sensor_ids);
    RUN_TEST(test_compact_existing_ux_fits_and_transport_failure_disables_only_display);
    return UNITY_END();
}
#endif
