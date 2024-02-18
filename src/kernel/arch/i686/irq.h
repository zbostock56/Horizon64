#pragma once
#include "isr.h"

typedef void (* IRQ_HANDLER)(REGISTERS *);

void i686_irq_init();
void i686_irq_register_handler(int, IRQ_HANDLER);