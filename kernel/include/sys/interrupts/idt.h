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

#include <structs/regs_str.h>
#include <structs/idt_str.h>

#include <common/memory.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
#define SET(x, flag) (x |= (flag))
#define UNSET(x, flag) (x &= ~(flag))

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void idt_init_entry(int interrupt, void *base, uint16_t segment, uint8_t type);
void idt_init();
void idt_enable_gate(int interrupt);
void idt_disable_gate(int interrupt);
