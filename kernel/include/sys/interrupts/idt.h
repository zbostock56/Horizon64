#pragma once

#include <globals.h>
#include <common/memory.h>

/* ----------------------------- STATIC GLOBALS ----------------------------- */

/* --------------------------------- DEFINES -------------------------------- */
#define SET(x, flag) (x |= (flag))
#define UNSET(x, flag) (x &= ~(flag))

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void idt_init_entry(int interrupt, void *base, uint16_t segment, uint8_t type);

/* --------------------------- EXTERNALLY DEFINED --------------------------- */
