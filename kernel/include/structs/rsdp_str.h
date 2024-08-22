/**
 * @file rsdp_str.h
 * @author Zack Bostock
 * @brief
 * RSDP (Root System Description Pointer) is a data structure used in the
 * ACPI programming interface.
 * @verbatim
 * The RSDP is either located within the first 1 KB of the EBDA
 * (Extended BIOS Data Area) (a 2 byte real mode segment pointer
 * to it is located at 0x40E), or in the memory region from 0x000E0000 to
 * 0x000FFFFF (the main BIOS area below 1 MB). TO find the table, the OS has to
 * find the "RSD PRT " string (the last space is important) in one of the two
 * areas. This signature is always on a 16 byte boundary.
 * @ref https://wiki.osdev.org/RSDP
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>

/**
 * @brief In ACPI Version 1.0 it has this structure
 */
typedef struct {
    char signature[8];
    uint8_t checksum;
    char oemid[6];
    uint8_t revision;
    uint32_t rsdt_address;
}  __attribute__((packed)) RSDP;

/**
 * @brief Since Version 2.0 it has been extended
 */
typedef struct {
    //RSDP rsdp;
    char signature[8];
    uint8_t checksum;
    char oemid[6];
    uint8_t revision;
    uint32_t rsdt_address;
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__((packed)) XSDP;
