#pragma once
#include <stdint.h>

void __attribute__((cdecl)) i686_panic();
uint8_t __attribute__((cdecl)) i686_inb(uint16_t);
void __attribute__((cdecl)) i686_outb(uint16_t, uint8_t);

uint8_t __attribute__((cdecl)) i686_enable_interrupts();
uint8_t __attribute__((cdecl)) i686_disable_interrupts();
void i686_io_wait();