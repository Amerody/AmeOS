#pragma once

#include <stdint.h>

void delay_ns(uint64_t ns);

static inline void delay_us(uint64_t us) { delay_ns(us * 1000u); }
static inline void delay_ms(uint64_t ms) { delay_ns(ms * 1000000u); }