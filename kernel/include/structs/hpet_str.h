/**
 * @file hpet_str.h
 * @author Zack Bostock
 * @brief Structures pertaining to the High Precision Event Timer (HPET)
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <structs/rsdt_str.h>
#include <structs/generic_addr_structure_str.h>


/**
 * @brief HPET struct from the OSDev Wiki
 * @ref https://wiki.osdev.org/HPET
 */
typedef struct {
    ACPI_SDT_HEADER header;
    uint8_t hw_rev_id;
    uint8_t comparator_count : 5;
    uint8_t counter_size : 1;
    uint8_t reserved : 1;
    uint8_t legacy_replacement : 1;
    uint16_t pci_vendor_id;
    GENERIC_ADDR_STRUCT address;
    uint8_t hpet_number;
    uint16_t minimum_tick;
    uint8_t page_protection;
} __attribute__((packed)) HPET_SDT;

typedef struct {
    volatile uint64_t config_and_capabilities;
    volatile uint64_t comparator_value;
    volatile uint64_t fsb_interrupt_route;
    volatile uint64_t unused;
} __attribute__((packed)) HPET_TIMER;

typedef struct {
    volatile uint64_t general_capabilities;
    volatile uint64_t unused_0;
    volatile uint64_t general_config;
    volatile uint64_t unused_1;
    volatile uint64_t general_int_status;
    volatile uint8_t unused_2[200];
    volatile uint64_t main_counter_value;
    volatile uint64_t unused_3;
    HPET_TIMER timers[];
} __attribute__((packed)) HPET;
