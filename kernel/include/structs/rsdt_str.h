/**
 * @file rsdt_str.h
 * @author Zack Bostock
 * @brief
 * RSDT (Root System Description Table) is a data structure used in
 * the ACPI programming interface. This table contains pointers to all the
 * other System Description Tables.
 * @ref https://wiki.osdev.org/RSDT
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>

typedef struct {
  char signature[4];
  uint32_t length;
  uint8_t revision;
  uint8_t checksum;
  char oemid[6];
  char oem_table_id[8];
  uint32_t oem_revision;
  uint32_t creator_id;
  uint32_t creator_revision;
} __attribute__((packed)) ACPI_SDT_HEADER;

typedef struct {
    ACPI_SDT_HEADER header;
    uint8_t data[];
}  __attribute__((packed)) ACPI_SDT;
