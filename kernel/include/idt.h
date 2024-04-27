#pragma once

#include <globals.h>
#include <memory.h>

#define SET(x, flag) (x |= (flag))
#define UNSET(x, flag) (x &= ~(flag))

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void idt_init_entry(int interrupt, void *base, uint16_t segment, uint8_t type);

/* --------------------------- EXTERNALLY DEFINED --------------------------- */
