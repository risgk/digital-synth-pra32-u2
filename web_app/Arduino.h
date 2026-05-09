#pragma once

#include <stdint.h>
#include <stddef.h>

typedef bool boolean;
typedef unsigned char byte;

#define PROGMEM
#define __not_in_flash_func(func_name) func_name

#define HIGH 1
#define LOW 0

#ifdef __cplusplus
extern "C" {
#endif

void digitalWrite(uint8_t pin, uint8_t val);

#ifdef __cplusplus
}
#endif
