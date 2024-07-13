/**
 * @file idt.h
 * @author Zack Bostock 
 * @brief Information pertaining to the Interrupt Descriptor Table 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <globals.h>
#include <common/memory.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
#define SET(x, flag) (x |= (flag))
#define UNSET(x, flag) (x &= ~(flag))

/* ----------------------------- STATIC GLOBALS ----------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void idt_init_entry(int interrupt, void *base, uint16_t segment, uint8_t type);

/* --------------------------- EXTERNALLY DEFINED --------------------------- */
