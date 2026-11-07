#pragma once

#include <stdint.h>

void* memcpy(void* dest, const void* src, uint32_t n);
void* memset(void* s, int c, uint32_t n);
void* memmove(void* dest, const void* src, uint32_t n);