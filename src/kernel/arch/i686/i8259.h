#pragma once

#include <stdint.h>
#include "pic.h"

void i8259_mask(int irq);
void i8259_unmask(int irq);