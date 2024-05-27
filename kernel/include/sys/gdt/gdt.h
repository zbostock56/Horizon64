#pragma once

#include <globals.h>
#include <common/memory.h>

/* ----------------------------- STATIC GLOBALS ----------------------------- */

/* --------------------------------- DEFINES -------------------------------- */
/* Locations within the GDT to specfic segments */
#define GDT_KERNEL_CODE_16_BIT      (0x8)
#define GDT_KERNEL_DATA_16_BIT      (0x10)
#define GDT_KERNEL_CODE_32_BIT      (0x18)
#define GDT_KERNEL_DATA_32_BIT      (0x20)
#define GDT_KERNEL_CODE_64_BIT      (0x28)
#define GDT_KERNEL_DATA_64_BIT      (0x30)
#define GDT_USER_CODE_64_BIT        (0x38)
#define GDT_USER_DATA_64_BIT        (0x40)
#define GDT_TSS                     (0x48)

/* --------------------------------- MACROS --------------------------------- */
#define GDT_LIMIT_LOW(limit)                (limit & 0xFFFF)
#define GDT_BASE_LOW(base)                  (base & 0xFFFF)
#define GDT_BASE_MID(base)                  ((base >> 16) & 0xFF)
#define GDT_FLAGS_LIMIT_HIGH(limit, flags)  (((limit >> 16) & 0xF) | \
                                            (flags & 0xF0))
#define GDT_BASE_HIGH(base)                 ((base >> 24) & 0xFF)

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void gdt_init_entry(GDT_ENTRY *entry, uint64_t base, uint64_t limit, 
                           uint8_t access, uint8_t flags);
void gdt_init(/* CPU *cpu_info */);

/* --------------------------- EXTERNALLY DEFINED --------------------------- */
void gdt_load(GDT_DESCRIPTOR *descriptor);