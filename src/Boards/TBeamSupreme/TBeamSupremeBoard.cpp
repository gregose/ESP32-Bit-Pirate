#ifdef DEVICE_TBEAM_SUPREME_S3
#include "TBeamSupremeBoard.h"
#include "TBeamSupremeConfig.h"
#include "TBeamSupremePower.h"
#include <Wire.h>
#include <XPowersLib.h>

namespace {
XPowersAXP2101 pmu;
bool pmuReady = false;

// I2C slave mode borrows Wire1, so restore the PMU bus before each rail change.
bool pmuBusReady() {
    if (!pmuReady) return false;
    Wire1.setTimeOut(50);
    return Wire1.begin(42, 41, 100000);
}
}

bool TBeamSupreme::setGnssEnabled(bool on) {
    if (!on) {
        // Release the lines first so an unpowered receiver is not back-fed.
        pinMode(gnssWakePin, INPUT);
        pinMode(gnssTxPin, INPUT);
        const bool off = pmuBusReady() && setGnssPower(pmu, false);
        gnssPowered = !off;
        if (!off) {
            pinMode(gnssWakePin, OUTPUT);
            digitalWrite(gnssWakePin, HIGH);
        }
        return off;
    }
    if (!pmuBusReady() || !setGnssPower(pmu, true)) {
        const bool off = pmuBusReady() && setGnssPower(pmu, false);
        gnssPowered = !off;
        if (gnssPowered) {
            pinMode(gnssWakePin, OUTPUT);
            digitalWrite(gnssWakePin, HIGH);
        }
        return false;
    }
    pinMode(gnssWakePin, OUTPUT);
    digitalWrite(gnssWakePin, HIGH);
    gnssPowered = true;
    return true;
}

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
    TBeamSupreme::gnssPowered = false;
    pmuReady = false;
    Wire1.setTimeOut(50);
    if (!Wire1.begin(42, 41, 100000) ||
        !pmu.begin(Wire1, AXP2101_SLAVE_ADDRESS, 42, 41)) {
        Serial.println("T-Beam Supreme: PMU setup failed. OLED, LoRa and SD disabled.");
        return;
    }
    const auto power = TBeamSupreme::configurePower(pmu);
    pmuReady = power.unusedRailsOff;
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
