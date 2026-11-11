#pragma once

#include <cstdint>

// Open-drain transport never remaps Wire or Wire1.
template <typename Io>
class SoftwareI2cDisplayBus {
public:
    SoftwareI2cDisplayBus(Io& io, uint8_t sda, uint8_t scl) : io(io), sda(sda), scl(scl) {}

    bool start(uint8_t address, bool read = false) {
        io.release(sda);
        if (!clockHigh() || !io.read(sda)) return fail();
        io.low(sda);
        io.pause();
        io.low(scl);
        return write(static_cast<uint8_t>((address << 1) | (read ? 1 : 0)));
    }

    bool write(uint8_t byte) {
        for (unsigned bit = 0; bit < 8; ++bit) {
            if (byte & 0x80) io.release(sda);
            else io.low(sda);
            io.pause();
            if (!clockHigh()) return fail();
            io.low(scl);
            byte <<= 1;
        }
        io.release(sda);
        if (!clockHigh()) return fail();
        const bool acknowledged = !io.read(sda);
        io.low(scl);
        return acknowledged || fail();
    }

    bool stop() {
        io.low(sda);
        io.pause();
        const bool clockOk = clockHigh();
        io.release(sda);
        io.pause();
        return clockOk && io.read(sda);
    }

    bool readByte(uint8_t& value) {
        value = 0;
        io.release(sda);
        for (unsigned bit = 0; bit < 8; ++bit) {
            if (!clockHigh()) return fail();
            value = static_cast<uint8_t>((value << 1) | (io.read(sda) ? 1 : 0));
            io.low(scl);
            io.pause();
        }
        // NACK terminates the single-byte read.
        if (!clockHigh()) return fail();
        io.low(scl);
        return true;
    }

    bool readRegister(uint8_t address, uint8_t reg, uint8_t& value) {
        const bool sent = start(address) && write(reg);
        const bool stopped = stop();
        if (!sent || !stopped) return false;
        const bool received = start(address, true) && readByte(value);
        return stop() && received;
    }

private:
    Io& io;
    const uint8_t sda, scl;

    bool clockHigh() {
        io.release(scl);
        // Bounded clock stretching: a missing/stuck display cannot strand USB.
        for (unsigned wait = 0; wait < 100; ++wait) {
            if (io.read(scl)) {
                io.pause();
                return true;
            }
            io.pause();
        }
        return false;
    }

    bool fail() {
        io.release(sda);
        io.release(scl);
        return false;
    }
};
