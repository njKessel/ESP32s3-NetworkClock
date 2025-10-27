#pragma once
#include <stdint.h>

namespace displayInit {
    bool begin(uint8_t sda, uint8_t scl, uint8_t address = 0x70, uint8_t brightness = 15);

    void setBrightness(uint8_t level);

    void clear();

    void showNumber(uint32_t value, uint8_t base = 10, bool leadingZeros = false, bool rightAlign = true, bool showColon = false);
}