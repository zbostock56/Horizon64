/**
 * @file alloc.h
 * @author Zack Bostock
 * @brief Function prototypes related to alloc
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once

#include <stddef.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
#define ALLOC_MAX_SIZE (65536U)

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void alloc_init();
void *alloc(uint64_t s);
void *realloc(void *addr, uint64_t s);
void free(void *addr);