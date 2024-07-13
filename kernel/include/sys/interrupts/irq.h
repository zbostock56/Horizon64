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

/* ---------------------------- LITERAL CONSTANTS --------------------------- */


/* ----------------------------- STATIC GLOBALS ----------------------------- */
static const PIC_DRIVER *pic = NULL;
static const PIT_DRIVER *pit = NULL;

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void irq_handler(REGISTERS *regs);
void irq_register_handler(int irq, IRQ_HANDLER handler);

/* --------------------------- EXTERNALLY DEFINED --------------------------- */
const PIC_DRIVER *pic_get_driver();
const PIT_DRIVER *pit_get_driver();
void clkhandler();
void isr_register_handler(int interrupt, ISR_HANDLER handler);