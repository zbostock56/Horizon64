#pragma once
#include "isr.h"

typedef void (*irq_handler)(REGISTERS *);

void i686_irq_init();
void i686_irq_register_handler(int, irq_handler);