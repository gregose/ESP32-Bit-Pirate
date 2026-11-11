#pragma once

namespace TBeamSupreme {
struct PowerStatus {
    bool unusedRailsOff = false;
    bool radio = false;
    bool sd = false;
    bool display = false;
};

// The PMU is already probed. Keep DC1 and the battery charging policy untouched.
template <typename Pmu>
PowerStatus configurePower(Pmu& pmu) {
    PowerStatus status;
    const bool displayOff = pmu.disableALDO1();
    const bool gpsOff = pmu.disableALDO4();
    const bool headerOff = pmu.disableBLDO2();
    status.unusedRailsOff = gpsOff && headerOff &&
        !pmu.isEnableALDO4() && !pmu.isEnableBLDO2();

    const bool radioOff = pmu.disableALDO3();
    const bool sdOff = pmu.disableBLDO1();
    if (!status.unusedRailsOff) return status;

    status.display = displayOff && !pmu.isEnableALDO1() &&
        pmu.setALDO1Voltage(3300) && pmu.getALDO1Voltage() == 3300 &&
        pmu.enableALDO1() && pmu.isEnableALDO1();
    status.radio = radioOff && pmu.setALDO3Voltage(3300) &&
        pmu.getALDO3Voltage() == 3300 && pmu.enableALDO3() && pmu.isEnableALDO3();
    status.sd = sdOff && pmu.setBLDO1Voltage(3300) &&
        pmu.getBLDO1Voltage() == 3300 && pmu.enableBLDO1() && pmu.isEnableBLDO1();
    if (!status.radio) pmu.disableALDO3();
    if (!status.sd) pmu.disableBLDO1();
    if (!status.display) pmu.disableALDO1();
    return status;
}

// GNSS rail (ALDO4) is enabled only while the onboard GNSS is in use.
template <typename Pmu>
bool setGnssPower(Pmu& pmu, bool on) {
    if (!on) return pmu.disableALDO4() && !pmu.isEnableALDO4();
    const bool enabled = pmu.setALDO4Voltage(3300) && pmu.getALDO4Voltage() == 3300 &&
        pmu.enableALDO4() && pmu.isEnableALDO4();
    if (!enabled) {
        pmu.disableALDO4();
        return false;
    }
    return enabled;
}
}
