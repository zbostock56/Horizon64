/**
 * @file alloc.h
 * @author Zack Bostocj
 * @brief Function prototypes related to alloc
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once

#include <stddef.h>

#define ALLOC_MAX_SIZE (16384U)

void alloc_init();
void *alloc(uint64_t s);
void *realloc(void *addr, uint64_t s);
void free(void *addr);