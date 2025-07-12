/**
 * @file symbols_str.h
 * @author Zack Bostock
 * @brief Used for backtracing when the kernel crashes
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>

typedef struct {
    uint64_t addr;
    char *name;
} SYMBOL;