/**
 * @file gdt_str.h
 * @author Zack Bostock 
 * @brief Structs for the Global Descriptor Table functionality 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <stdint.h>

/*
    GDT_ENTRY
    Limit (bits 0 - 15)
    Base (bits 0 - 15)
    Base (bits 16 - 32)
    Access Byte
    Limit (bits 16 - 19) OR flags
    Base (bits 24 - 31)
*/

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t flags_limit_high;
    uint8_t base_high;
} __attribute__((packed)) GDT_ENTRY;

typedef struct {
    uint16_t size;
    uint64_t offset;
} __attribute__((packed)) GDT_DESCRIPTOR;

typedef struct {
    /* Define each attribute as a bitfield */
    uint32_t segment_limit_low   : 16;
    uint32_t segment_base_low    : 16;
    uint32_t segment_base_mid    : 8;
    uint32_t segment_type        : 4;
    uint32_t zero_1              : 1;
    uint32_t segment_dpl         : 2;
    uint32_t segment_present     : 1;
    uint32_t segment_limit_high  : 4;
    uint32_t segment_avail       : 1;
    uint32_t zero_2              : 2;
    uint32_t segment_granulatity : 1;
    uint32_t segment_base_mid_2  : 8;
    uint32_t segment_base_high   : 32;
    uint32_t _reserved_1         : 8;
    uint32_t zero_3              : 5;
    uint32_t _reserved_2         : 19;
} __attribute__((packed)) SYSTEM_SEGMENT_SELECTOR;

typedef struct {
    GDT_ENTRY null_desc;
    GDT_ENTRY kernel_code_16_bit;
    GDT_ENTRY kernel_data_16_bit;
    GDT_ENTRY kernel_code_32_bit;
    GDT_ENTRY kernel_data_32_bit;
    GDT_ENTRY kernel_code_64_bit;
    GDT_ENTRY kernel_data_64_bit;
    GDT_ENTRY user_code_64_bit;
    GDT_ENTRY user_data_64_bit;
    //SYSTEM_SEGMENT_SELECTOR tss;
} __attribute__((packed)) GDT_TABLE;

typedef enum {
    GDT_FLAG_64BIT                      = 0x20,
    GDT_FLAG_32BIT                      = 0x40,
    GDT_FLAG_16BIT                      = 0x00,

    GDT_FLAG_GRANULARITY_1B             = 0x00,
    GDT_FLAG_GRANULARITY_4K             = 0x80,
} GDT_FLAGS;

typedef enum {
    GDT_ACCESS_CODE_READABLE            = 0x02,
    GDT_ACCESS_DATA_WRITEABLE           = 0x02,
    GDT_ACCESS_EXECUTABLE               = 0x08,

    GDT_ACCESS_CODE_CONFORMING          = 0x04,
    
    GDT_ACCESS_DATA_DIRECTION_DOWN      = 0x04,
    GDT_ACCESS_DATA_DIRECTION_UP        = 0x00,

    GDT_ACCESS_DATA_SEGMENT             = 0x10,
    GDT_ACCESS_CODE_SEGMENT             = 0x10,

    GDT_ACCESS_DESCRIPTOR_TSS           = 0x00,

    GDT_ACCESS_RING0                    = 0x00,
    GDT_ACCESS_RING1                    = 0x20,
    GDT_ACCESS_RING2                    = 0x40,
    GDT_ACCESS_RING3                    = 0x60,

    GDT_ACCESS_PRESENT                  = 0x80,
} GDT_ACCESS_BYTE;