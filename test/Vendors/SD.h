#pragma once

#include <cstddef>
#include <cstdint>

namespace fs {

class File {
public:
    File() = default;
    explicit File(bool open, size_t size = 0) : opened(open), fileSize(size) {}

    explicit operator bool() const { return opened; }
    size_t size() const { return fileSize; }
    void close() { opened = false; }
#ifdef TBEAM_TEST_STANDALONE
    bool isDirectory() const { return false; }
    File openNextFile() { return File(); }
    const char* name() const { return ""; }
    bool available() const { return false; }
    int read() { return -1; }
    bool seek(size_t) { return false; }
    size_t readBytes(char*, size_t) { return 0; }
    size_t write(const uint8_t*, size_t) { return 0; }
#endif

private:
    bool opened = false;
    size_t fileSize = 0;
};

}  // namespace fs

using fs::File;
