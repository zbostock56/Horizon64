/**
 * @file idt_str.h
 * @author Zack Bostock
 * @brief Structs pertaining to Interrupt Descriptor Table functionality
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>

#define X86_64_IDT_ENTRIES (256)

/* 16-bit offset for each entry */

/* Each of these represents a 64-bit entry */
typedef struct {
    uint16_t base_low;
    uint16_t segment_selector;  /* Which code segment from GDT is being used */
    uint8_t ist;                /* Set to zero for now */
    uint8_t type;               /* Types and attributes */
    uint16_t base_mid;
    uint32_t base_high;
    uint32_t _reserved;         /* Set to zero */
} __attribute__((packed)) IDT_ENTRY;

typedef struct {
    uint16_t limit;
    uint64_t size;
} __attribute__((packed)) IDT_DESCRIPTOR;

typedef enum {
    IDT_FLAG_GATE_64BIT_INT         = 0x8E,
    IDT_FLAG_GATE_64BIT_TRAP        = 0x8F,

    IDT_FLAG_GATE_TASK              = 0x5,
    IDT_FLAG_GATE_16BIT_INT         = 0x6,
    IDT_FLAG_GATE_16BIT_TRAP        = 0x7,
    IDT_FLAG_GATE_32BIT_INT         = 0xE,
    IDT_FLAG_GATE_32BIT_TRAP        = 0xF,

    IDT_FLAG_RING0                  = (0 << 5),
    IDT_FLAG_RING1                  = (1 << 5),
    IDT_FLAG_RING2                  = (2 << 5),
    IDT_FLAG_RING3                  = (3 << 5),

    IDT_FLAG_PRESENT                = 0x80,
} IDT_FLAGS;
