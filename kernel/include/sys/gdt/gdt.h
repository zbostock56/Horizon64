/**
 * @file gdt.h
 * @author Zack Bostock
 * @brief Information pertaining to the Global Descriptor Table
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <globals.h>

#include <structs/gdt_str.h>

#include <common/memory.h>

#include <sys/asm.h>
#include <sys/smp.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */

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
void gdt_init(CPU *cpu_info);
void gdt_init_tss(CPU *cpu_info);

/* --------------------------- EXTERNALLY DEFINED --------------------------- */
void gdt_load(GDT_DESCRIPTOR *descriptor);
