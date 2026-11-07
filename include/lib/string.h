#pragma once

#include <stdint.h>

uint32_t strlen(const char* s);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, uint32_t n);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, uint32_t n);