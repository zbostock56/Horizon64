/**
 * @file irq.h
 * @author Zack Bostock
 * @brief Information pertaining to hardware interrupts
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <globals.h>

#include <structs/pic_str.h>
#include <structs/pit_str.h>
#include <structs/regs_str.h>

#include <sys/asm.h>
#include <sys/tick/pic.h>
#include <sys/tick/pit.h>
#include <sys/interrupts/isr.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void irq_handler(REGISTERS *regs);
void irq_register_handler(int irq, IRQ_HANDLER handler);
void irq_init();

/* --------------------------- EXTERNALLY DEFINED --------------------------- */
void clkhandler();
