/**
 * @file madt.h
 * @author Zack Bostock
 * @brief All information pertaining to the Multiple APIC Descriptor Table
 * (MADT)
 * @ref https://wiki.osdev.org/MADT
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <globals.h>

#include <structs/madt_str.h>

#include <sys/asm.h>
#include <sys/acpi/acpi.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
#define MADT_RECORD_TYPE_LOCAL_APIC                     (0)
#define MADT_RECORD_TYPE_IO_APIC                        (1)
#define MADT_RECORD_TYPE_INT_SRC_OVERRIDE               (2)
#define MADT_RECORD_TYPE_NON_MASKABLE_INT_SRC           (3)
#define MADT_RECORD_TYPE_NON_MASKABLE_INT               (4)
#define MADT_RECORD_TYPE_TYPE_LOCAL_APIC_ADDR_OVERRIDE  (5)

#define MAX_IO_APICS    (2)

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */
#define MADT_LAPIC_FLAG_ENABLED                         (1 << 0)
#define MADT_LAPIC_FLAG_ONLINE_CAPABLE                  (1 << 1)
#define MADT_LAPIC_ACTIVE_LOW(flags)                    ((flags & 2))
#define MADT_LAPIC_LEVEL_TRIGGERED(flags)               ((flags & 8))

/* --------------------------- INTERNALLY DEFINED --------------------------- */
uint32_t madt_get_num_ioapics();
uint32_t madt_get_num_local_apics();
MADT_RECORD_IO_APIC **madt_get_io_apics();
MADT_RECORD_LAPIC **madt_get_local_apics();
uint64_t madt_get_local_apic_base();
void madt_init();
