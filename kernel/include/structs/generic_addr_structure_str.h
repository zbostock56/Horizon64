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

/**
 * @brief Possible vaule of the address_space field
 */
#define GAS_SYS_MEM                         (0x0)
#define GAS_SYS_IO                          (0x1)
#define GAS_PCI_CONFIG_SPACE                (0x2)
#define GAS_EMBEDDED_CONTROLLER             (0x3)
#define GAS_SYSTEM_MNGMT_BUS                (0x4)
#define GAS_SYS_CMOS                        (0x5)
#define GAS_PCI_BAR_TARGET                  (0x6)
#define GAS_INTELLIGENT_PLTFM_MNGMT_INFR    (0x7)
#define GAS_GEN_PURPOSE_IO                  (0x8)
#define GAS_GEN_SERIAL_BUS                  (0x9)
#define GAS_PLTFM_COM_CHANNEL               (0xA)

/**
 * @brief Possible value of the access_size field
 */
#define GAS_ACC_SZ_UNDEFINED                (0x0)
#define GAS_ACC_SZ_BYTE_ACCESS              (0x1)
#define GAS_ACC_SZ_WORD_ACCESS              (0x2)
#define GAS_ACC_SZ_DWORD_ACCESS             (0x3)
#define GAS_ACC_SZ_QWORD_ACCESS             (0x4)

typedef struct {
    uint8_t address_space;
    uint8_t bit_width;
    uint8_t bit_offset;
    uint8_t access_size;
    uint64_t address;
} GENERIC_ADDR_STRUCT;
