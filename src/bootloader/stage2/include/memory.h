#pragma once
#include <stdint.h>

void *memcpy(void *, const void *, uint16_t);
void *memset(void *, int, uint16_t);
int memcmp(const void *, const void *, uint16_t);
void *segoffset_to_linear(void *);