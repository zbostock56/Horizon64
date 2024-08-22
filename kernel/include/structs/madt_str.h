/**
 * @file madt_str.h
 * @author Zack Bostock
 * @brief The Multiple ACPI Description Table (MADT) is an ACPI table which
 * provides info necessary for operation on systems with ACPI.
 * @ref https://wiki.osdev.org/MADT
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>

#include <structs/acpi_str.h>

/* lapic => local APIC */
/* gsi => global system interrupt */

/**
 * @brief MADT Record header. This prepends all the other entries and denotes
 * what the rest of the data in the structure is.
 */
typedef struct {
    uint8_t type;
    uint8_t length;
} __attribute__((packed)) MADT_RECORD_HEADER;

/**
 * @brief Single logical processor and its local interrupt controller
 * Entry Type: 0
 */
typedef struct {
    MADT_RECORD_HEADER header;

    uint8_t process_id;
    uint8_t apic_id;
    uint32_t flags;
} __attribute__((packed)) MADT_RECORD_LAPIC;

/**
 * @brief I/O APIC
 * Entry Type: 1
 */
typedef struct {
    MADT_RECORD_HEADER header;

    uint8_t id;
    uint8_t reserved;
    uint32_t addr;
    uint32_t gsi_base;
} __attribute__((packed)) MADT_RECORD_IO_APIC;

/**
 * @brief I/O APIC Interrupt Source Override
 * Entry Type: 2
 */
typedef struct {
    MADT_RECORD_HEADER header;

    uint8_t bus_src;
    uint8_t irq_src;
    uint32_t gsi;
    uint16_t flags;
} __attribute__((packed)) MADT_RECORD_INT_SRC_OVERRIDE;

/**
 * @brief I/O APIC Non-maskable Interrupt Source
 * Entry Type: 3
 */
typedef struct {
    MADT_RECORD_HEADER header;

    uint8_t src;
    uint8_t reserved;
    uint16_t flags;
    uint32_t gsi;
} __attribute__((packed)) MADT_RECORD_NON_MASKABLE_INT_SRC;

/**
 * @brief Local APIC Non-maskable interrupts
 * Entry Type: 4
 */
typedef struct {
    MADT_RECORD_HEADER header;

    uint8_t acpi_processor_id;
    uint16_t flags;
    uint8_t l_int;
} __attribute__((packed)) MADT_RECORD_NON_MASKABLE_INT;

/**
 * @brief Local APIC Address Override
 * Entry Type: 5
 */
typedef struct {
    uint16_t reserved;
    uint64_t addr_apic;
} __attribute__((packed)) MADT_RECORD_APIC_ADDR_OVERRIDE;

/**
 * @brief MADT
 */
typedef struct {
    ACPI_SDT_HEADER header;

    uint32_t lapic_address;
    uint32_t flags;

    uint8_t records[];
} __attribute__((packed)) MADT;
