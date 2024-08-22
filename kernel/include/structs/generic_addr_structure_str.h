/**
 * @file generic_addr_structure_str.h
 * @author Zack Bostock
 * @brief Generic Address Structure (GAS) is a structure used by the ACPI to
 * describe the position of registers.
 * @ref https://wiki.osdev.org/FADT (at middle of page)
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>

typedef struct {
    uint8_t address_space;
    uint8_t bit_width;
    uint8_t bit_offset;
    uint8_t access_size;
    uint64_t address;
} GENERIC_ADDR_STRUCT;
