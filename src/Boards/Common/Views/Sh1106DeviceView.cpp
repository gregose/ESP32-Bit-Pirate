#if defined(DEVICE_TBEAM_SUPREME_S3) || (defined(DEVICE_CUSTOM) && defined(CUSTOM_DISPLAY_DRIVER_SH1106_SW_I2C))
#include "Sh1106DeviceView.h"
#include "States/GlobalState.h"
#include <algorithm>
#include <cstdio>
#include <new>

#if !defined(U8X8_WITH_USER_PTR) || !defined(U8G2_USE_DYNAMIC_ALLOC)
#error "SH1106 requires U8X8_WITH_USER_PTR and U8G2_USE_DYNAMIC_ALLOC in build_flags"
#endif

uint8_t Sh1106DeviceView::gpioDelay(u8x8_t*, uint8_t message, uint8_t count, void*) {
    if (message == U8X8_MSG_DELAY_MILLI) delay(count);
    return 1;
}

uint8_t Sh1106DeviceView::transfer(u8x8_t* screen, uint8_t message, uint8_t count, void* data) {
    auto* self = static_cast<Sh1106DeviceView*>(u8x8_GetUserPtr(screen));
    if (!self->transferOk) return 0;
    switch (message) {
        case U8X8_MSG_BYTE_START_TRANSFER:
            self->transferOk = self->bus.start(self->address);
            break;
        case U8X8_MSG_BYTE_SEND: {
            const auto* bytes = static_cast<const uint8_t*>(data);
            for (uint8_t i = 0; i < count && self->transferOk; ++i)
                self->transferOk = self->bus.write(bytes[i]);
            break;
        }
        case U8X8_MSG_BYTE_END_TRANSFER:
            self->transferOk = self->bus.stop();
            break;
        default: break;
    }
    return self->transferOk;
}

void Sh1106DeviceView::initialize() {
    if (attempted) return;
    attempted = true;
    if (!config.powered) {
        Serial.println("SH1106: display power unavailable; continuing without display.");
        return;
    }
    if (config.sda > 48 || config.scl > 48 || config.sda == config.scl ||
        (config.address != 0 && config.address != 0x3C && config.address != 0x3D)) {
        Serial.println("SH1106: invalid display pins/address; continuing without display.");
        return;
    }
    for (uint8_t candidate : {0x3C, 0x3D}) {
        if (config.address != 0 && candidate != config.address) continue;
        const bool acknowledged = bus.start(candidate);
        const bool stopped = bus.stop();
        if (!acknowledged || !stopped) continue;
        if (config.excludeQmc6310) {
            // Opt-in board policy: LilyGO's QMC6310N register-0 exclusion.
            uint8_t id = 0;
            if (!bus.readRegister(candidate, 0, id)) {
                // SH1106 may NACK reads; write ACK is still not identity proof.
                const bool writable = bus.start(candidate);
                const bool idle = bus.stop();
                if (!writable || !idle) {
                    Serial.println("SH1106: address check failed; continuing without display.");
                    return;
                }
                Serial.println("SH1106: candidate has no readable ID; visual confirmation required.");
            }
            if (id == 0x80) {
                Serial.println("SH1106: QMC6310 magnetometer excluded.");
                continue;
            }
        }
        if (address != 0) {
            Serial.println("SH1106: ambiguous addresses; configure display address 0x3C/0x3D.");
            return;
        }
        address = candidate;
    }
    if (address == 0) {
        Serial.println("SH1106: no display candidate; continuing with USB.");
        return;
    }
    buffer.reset(new (std::nothrow) uint8_t[1024]{});
    if (!buffer) {
        Serial.println("SH1106: framebuffer allocation failed; continuing without display.");
        return;
    }
    u8g2_Setup_sh1106_i2c_128x64_noname_f(display.getU8g2(),
        (config.rotation & 2) ? U8G2_R2 : U8G2_R0, transfer, gpioDelay);
    u8g2_SetBufferPtr(display.getU8g2(), buffer.get());
    u8x8_SetUserPtr(display.getU8x8(), this);
    display.setI2CAddress(address << 1);
    display.initDisplay();
    display.setPowerSave(0);
    display.setFont(u8g2_font_5x7_tf);
    ready = transferOk;
    if (!ready) {
        buffer.reset();
        Serial.println("SH1106: configured address NACK/bus timeout; USB remains available.");
        return;
    }
    Serial.println("SH1106: writes acknowledged; confirm display visually (ACK is not identity).");
    setBrightness(brightness);
    logo();
}

void Sh1106DeviceView::flush() {
    if (!ready) return;
    display.sendBuffer();
    if (!transferOk) {
        ready = false;
        buffer.reset();
        Serial.println("SH1106: transfer failed; display disabled until reboot, USB remains available.");
    }
}

void Sh1106DeviceView::copyText(std::string_view value, char (&output)[26], size_t columns) {
    // The existing labels are ASCII; omit multi-byte decorations on the tiny font.
    const size_t limit = std::min<size_t>(columns, sizeof(output) - 1);
    size_t length = 0;
    for (unsigned char c : value) {
        if (c < 32 || c >= 127) continue;
        if (length == limit) {
            if (length) output[length - 1] = '~';
            break;
        }
        output[length++] = static_cast<char>(c);
    }
    output[length] = '\0';
}

void Sh1106DeviceView::text(std::string_view value, int x, int row, size_t columns) {
    char ascii[26];
    copyText(value, ascii, columns);
    if (row >= 0 && row < 8) display.drawStr(x, row * 8 + 7, ascii);
}

void Sh1106DeviceView::frame(std::string_view heading) {
    display.clearBuffer();
    text(heading, 0, 0);
    display.drawHLine(0, 8, 128);
}

void Sh1106DeviceView::lines(std::string_view heading, const std::vector<std::string>& values) {
    if (!ready) return;
    frame(heading);
    for (size_t i = 0; i < values.size() && i < (values.size() > 6 ? 5 : 6); ++i) text(values[i], 0, i + 2);
    if (values.size() > 6) text("+ details on console", 0, 7);
    flush();
}

void Sh1106DeviceView::logo() {
    lines("ESP32 BIT PIRATE", {"ESP32 SWISS ARMY KNIFE", std::string("Version ") + GlobalState::getInstance().getVersion()});
}

void Sh1106DeviceView::welcome(TerminalTypeEnum& type, std::string& info) {
    if (type == TerminalTypeEnum::WiFiAp) {
        auto& state = GlobalState::getInstance();
        lines("HOTSPOT", {state.getActiveApName(), std::string("PW ") + state.getApPassword(), "IP " + info, "CONNECT TO AP"});
    } else if (type == TerminalTypeEnum::WiFiClient) {
        lines("Open browser to connect", {"http://" + info});
    } else {
        lines("Open Serial (USB COM)", {"Baudrate: " + info, "Then press any key", "in terminal"});
    }
}

void Sh1106DeviceView::show(PinoutConfig& config) { lines(config.getMode(), config.getMappings()); }
void Sh1106DeviceView::loading() { lines(title, {"Loading..."}); }

void Sh1106DeviceView::adapterMode(const std::string& name, const std::string& description,
                                       const std::vector<std::string>& details) {
    if (!ready) return;
    frame(name);
    // Eight adapter fields fit in two compact columns; console retains full text.
    for (size_t i = 0; i < details.size() && i < 8; ++i)
        text(details[i], (i % 2) * 65, 2 + i / 2, 12);
    text(description, 0, 6);
    text("Press button to return", 0, 7);
    flush();
}

void Sh1106DeviceView::clear() {
    if (!ready) return;
    display.clearBuffer();
    flush();
}

void Sh1106DeviceView::topBar(const std::string& heading, bool, bool) {
    copyText(heading, title);
    if (!ready) return;
    frame(title);
    flush();
}

void Sh1106DeviceView::horizontalSelection(const std::vector<std::string>& options, uint16_t selected,
                                                const std::string& first, const std::string& second) {
    if (selected >= options.size()) return;
    if (second == "Short press: CONNECT  Long press: HOTSPOT") {
        lines(title, {first, "< " + options[selected] + " >", "Short press: CONNECT", "Long press: HOTSPOT", "No input: Serial (3s)"});
    } else {
        lines(title, {first, "< " + options[selected] + " >", second});
    }
}

void Sh1106DeviceView::trace(uint8_t pin, const std::vector<uint8_t>& values, uint8_t step, bool analog) {
    if (!ready) return;
    frame("GPIO " + std::to_string(pin));
    const int stride = std::max<int>(1, step);
    for (size_t i = 1; i < values.size() && i * stride < 128; ++i) {
        const auto y = [analog](uint8_t v) { return analog ? 63 - (v * 47 / 255) : (v ? 23 : 55); };
        const int x = (i - 1) * stride, next = i * stride;
        if (analog) display.drawLine(x, y(values[i - 1]), next, y(values[i]));
        else {
            display.drawLine(x, y(values[i - 1]), next, y(values[i - 1]));
            display.drawLine(next, y(values[i - 1]), next, y(values[i]));
        }
    }
    flush();
}

void Sh1106DeviceView::drawLogicTrace(uint8_t pin, const std::vector<uint8_t>& v, uint8_t step) { trace(pin, v, step, false); }
void Sh1106DeviceView::drawAnalogicTrace(uint8_t pin, const std::vector<uint8_t>& v, uint8_t step) { trace(pin, v, step, true); }

void Sh1106DeviceView::drawWaterfall(const std::string& heading, float start, float end, const char* unit,
                                         int rowIndex, int rowCount, int level) {
    if (!ready || rowCount <= 1) return;
    if (rowIndex == 0) {
        frame(heading);
        char range[48];
        snprintf(range, sizeof(range), "%.2f-%.2f %s", start, end, unit ? unit : "");
        text(range, 0, 7);
    }
    const int y = 10 + (std::clamp(rowIndex, 0, rowCount - 1) * 44LL / (rowCount - 1));
    display.setDrawColor(0);
    display.drawHLine(0, y, 128);
    display.setDrawColor(1);
    const int width = std::clamp(level, 0, 100) * 128 / 100;
    if (width) display.drawHLine((128 - width) / 2, y, width);
    // One refresh per sweep, not one full I2C frame for every sampled frequency.
    if (rowIndex >= rowCount - 1) flush();
}

void Sh1106DeviceView::setRotation(uint8_t rotation) {
    if (!ready) return;
    display.setDisplayRotation((rotation & 2) ? U8G2_R2 : U8G2_R0);
    flush();
}

void Sh1106DeviceView::setBrightness(uint8_t value) {
    brightness = std::min<uint8_t>(value, 100);
    if (!ready) return;
    display.setContrast(brightness * 255 / 100);
    display.setPowerSave(brightness == 0);
    if (!transferOk) {
        ready = false;
        buffer.reset();
        Serial.println("SH1106: contrast/power command failed; display disabled until reboot.");
    }
}
#endif
