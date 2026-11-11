#ifdef DEVICE_TBEAM_SUPREME_S3
#include "TBeamSupremeBoard.h"
#include "TBeamSupremeConfig.h"
#include "TBeamSupremePower.h"
#include <Wire.h>
#include <XPowersLib.h>

void TBeamSupremeBoard::initialize() {
    Serial.begin(115200);
    // Both SPI slaves on the SD/IMU network must be deselected before clocking.
    for (uint8_t cs : {uint8_t(10), uint8_t(34), uint8_t(47)}) {
        pinMode(cs, OUTPUT);
        digitalWrite(cs, HIGH);
    }

    TBeamSupreme::radioReady = false;
    TBeamSupreme::displayReady = false;
    TBeamSupreme::sdReady = false;
    Wire1.setTimeOut(50);
    XPowersAXP2101 pmu;
    if (!Wire1.begin(42, 41, 100000) ||
        !pmu.begin(Wire1, AXP2101_SLAVE_ADDRESS, 42, 41)) {
        Serial.println("T-Beam Supreme: PMU not found. OLED, LoRa and SD disabled.");
        return;
    }
    const auto power = TBeamSupreme::configurePower(pmu);
    TBeamSupreme::radioReady = power.radio;
    TBeamSupreme::sdReady = power.sd;
    TBeamSupreme::displayReady = power.display;
    deviceView.setPowerAvailable(power.display);
    if (!power.unusedRailsOff) {
        Serial.println("T-Beam Supreme: Rail setup failed. LoRa and SD disabled.");
    }
    if (!power.radio) Serial.println("T-Beam Supreme: LoRa power failed. LoRa disabled.");
    if (!power.sd) Serial.println("T-Beam Supreme: SD power failed. SD disabled.");
    if (!power.display) Serial.println("T-Beam Supreme: OLED power failed. OLED disabled.");
    delay(250);
    Serial.printf("T-Beam Supreme: Flash %u bytes, PSRAM %u bytes, LoRa %s, SD %s\n",
                  ESP.getFlashChipSize(), ESP.getPsramSize(),
                  power.radio ? "ready" : "disabled", power.sd ? "ready" : "disabled");
    deviceView.initialize();
}
#endif
