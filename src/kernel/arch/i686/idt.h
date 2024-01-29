#pragma once

#include <stdint.h>

#define i686_IDT_ENTRIES (256)

typedef struct {
    uint16_t base_low;
    uint16_t segment_selector;
    uint8_t _reserved;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed)) IDT_ENTRY;

typedef struct {
    uint16_t limit;
    IDT_ENTRY *entry;
} __attribute__((packed)) IDT_DESCRIPTOR;

typedef enum {
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

void i686_idt_init();
void i686_idt_disable_gate(int);
void i686_idt_enable_gate(int);
void i686_idt_set_gate(int, void *, uint16_t, uint8_t);