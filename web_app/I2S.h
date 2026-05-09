#pragma once
#include <stdint.h>

class I2SClass {
public:
    void write(int32_t sample) {}
};

extern I2SClass g_i2s_output;
