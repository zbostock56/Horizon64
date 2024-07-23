/**
 * @file memory_map_str.h
 * @author Zack Bostock
 * @brief Struct information for holding memory maps
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>

typedef struct {
    uint64_t virt_addr;
    uint64_t phys_addr;
    uint64_t flags;
    uint64_t num_pages;
} MEM_MAP;
