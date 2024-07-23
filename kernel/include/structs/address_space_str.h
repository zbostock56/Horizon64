/**
 * @file address_space_str.h
 * @author Zack Bostock
 * @brief Defines a memory address space as a struct
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <common/lock.h>
#include <common/vector.h>

typedef struct {
    uint64_t *pml4;
    vector_struct(uint64_t) memory_list;
    LOCK lock;
} ADDR_SPACE;
