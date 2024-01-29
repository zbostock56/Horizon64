#pragma once

#include <stdint.h>

void __attribute__((cdecl)) x86_outb(uint16_t, uint8_t );
uint8_t __attribute__((cdecl)) x86_inb(uint16_t );