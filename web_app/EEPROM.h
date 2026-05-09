#pragma once
#include <stdint.h>

class EEPROMClass {
public:
    void begin(int size) {}
    uint8_t read(int address) { return 0; }
    void write(int address, uint8_t val) {}
    void commit() {}
};

extern EEPROMClass EEPROM;
