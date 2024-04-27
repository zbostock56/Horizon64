#pragma once

#include <stdint.h>

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void outb(int16_t port, uint8_t val);
uint8_t inb(uint16_t port);
void io_wait();
void halt();
void enable_interrupts();
void disable_interrupts();

/* --------------------------- EXTERNALLY DEFINED --------------------------- */
