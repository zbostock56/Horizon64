#pragma once

#include <globals.h>
#include <structs/pic_str.h>
#include <structs/pit_str.h>

static const PIC_DRIVER *pic = NULL;
static const PIT_DRIVER *pit = NULL;

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void irq_handler(REGISTERS *regs);
void irq_register_handler(int irq, IRQ_HANDLER handler);

/* --------------------------- EXTERNALLY DEFINED --------------------------- */
const PIC_DRIVER *pic_get_driver();
const PIT_DRIVER *pit_get_driver();
void clkhandler();
void isr_register_handler(int interrupt, ISR_HANDLER handler);