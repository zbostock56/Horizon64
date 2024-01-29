#pragma once

#include <stdint.h>

void __attribute__((cdecl)) x86_panic();
void __attribute__((cdecl)) x86_outb(uint16_t, uint8_t );
uint8_t __attribute__((cdecl)) x86_inb(uint16_t );
uint8_t  __attribute__((cdecl)) x86_disk_get_drive_params(uint8_t, uint8_t *, 
                                                        uint16_t *, uint16_t *,
                                                        uint16_t *);
uint8_t __attribute__((cdecl)) x86_disk_reset(uint8_t drive);
uint8_t __attribute__((cdecl)) x86_disk_read(uint8_t, uint16_t, uint16_t,
                                          uint16_t, uint8_t, void *);

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;
} E820_MEMORY_BLOCK;

enum E820_MEMORY_BLOCK_TYPE {
    E820_USABLE = 1,
    E820_RESERVED = 2,
    E820_ACPI_RECLAIMABLE = 3,
    E820_ACPI_NVS = 4,
    E820_BAD_MEMORY = 5,
};

int x86_E820_get_next_block(E820_MEMORY_BLOCK *, uint32_t *);