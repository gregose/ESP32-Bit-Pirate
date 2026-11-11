#pragma once

#ifdef DEVICE_TBEAM_SUPREME_S3
#include "Boards/Common/Views/Sh1106DeviceView.h"
#include "Boards/Common/Inputs/DefaultInput.h"
#include "Boards/Common/Serial/BoardHostSerial.h"

class TBeamSupremeBoard final {
public:
    void initialize();
    IDeviceView& getDeviceView() { return deviceView; }
    IInput& getDeviceInput() { return deviceInput; }
    IHostSerial& getHostSerial() { return hostSerial; }

private:
    BoardHostSerial hostSerial;
#ifndef TBEAM_OLED_ADDRESS
#define TBEAM_OLED_ADDRESS 0
#endif
    Sh1106DeviceView deviceView{Sh1106Config{17, 18, TBEAM_OLED_ADDRESS, false, true, 2}};
    DefaultInput deviceInput;
};
#endif
