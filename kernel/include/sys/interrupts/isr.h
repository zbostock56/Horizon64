/**
 * @file isr.h
 * @author Zack Bostock
 * @brief Information pertaining to Interrupt Service Routines
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <globals.h>
#include <const.h>

#include <structs/regs_str.h>
#include <structs/idt_str.h>

#include <sys/asm.h>
#include <sys/interrupts/idt.h>

#include <util/stack_walk.h>
#include <util/walk_memory.h>

/* -------------------------------- GLOBALS --------------------------------- */

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void isr_handler(REGISTERS *regs);
void isr_init();
void isr_register_handler(int interrupt, ISR_HANDLER handler);
