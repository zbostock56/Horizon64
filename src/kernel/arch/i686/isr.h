#pragma once

#include <stdint.h>
#include "general.h"

typedef void (* ISR_HANDLER)(REGISTERS *);

void i686_isr_init();
void i686_isr_register_handler(int, ISR_HANDLER);