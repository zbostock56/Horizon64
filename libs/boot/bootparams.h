#pragma once

#include <stdint.h>

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;
} MEMORY_REGION;

typedef struct {
    int region_count;
    MEMORY_REGION *regions;
} MEMORY_INFO;

/* Passed from stage2 bootloader to kernel when stage2 gives control to */
/* kernel. Holds necessary information about memory map for legacy BIOS */
typedef struct {
    MEMORY_INFO memory;
    uint8_t boot_device;
} BOOT_PARAMS;
