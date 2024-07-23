/**
 * @file asm.h
 * @author Zack Bostock
 * @brief Information pertaining to c-wrapped assembly functions
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void outb(int16_t port, uint8_t val);
uint8_t inb(uint16_t port);
void io_wait();
void halt();
void enable_interrupts();
void disable_interrupts();
