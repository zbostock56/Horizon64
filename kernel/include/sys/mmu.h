/**
 * @file mmu.h
 * @author Zack Bostock 
 * @brief Contains information pertaining to the kernel's memory management
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <globals.h>
#include <structs/mmu_str.h>
#include <common/memory.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
#define PAGES_PER_BYTE  (8)
#define PAGE_SIZE       (4096)

/* Determed from the HHDM offset input from bootloader */
#define MEM_VIRT_OFFSET (0xFFFF800000000000)
#define FREE            (1)
#define ALLOCATED       (0)

typedef uint8_t BITMAP_STATUS;

/* ----------------------------- STATIC GLOBALS ----------------------------- */
static KERNEL_MEM_INFO kmem = {0};

/* --------------------------------- MACROS --------------------------------- */
#define ENTRY_TYPE_CHECK(entry)                                 \
      (entry->type == LIMINE_MEMMAP_USABLE ||                   \
      entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE ||    \
      entry->type == LIMINE_MEMMAP_ACPI_RECLAIMABLE)            \

#define PRINT_MEM_SIZE(size) {                                  \
  klogi("%d -- B\n%d -- KB\n"                                   \
                  "%d -- MB\n",                                 \
                  size, size / 1024, size / 1024 / 1024);       \
}

#define PRINT_MEM_ENTRY_INFO(entry) {                           \
  klogi("Physical Memory Entry\n"                               \
                          "Base: %x (%d)\nLength: %x (%d)\n",   \
                          entry->base, entry->base,             \
                          entry->length, entry->length);        \
  switch (entry->type) {                                        \
    case LIMINE_MEMMAP_USABLE:                                  \
      klogi("Type: LIMINE_MEMMAP_USABLE\n");                    \
      break;                                                    \
    case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:                  \
      klogi(                                                    \
        "Type: LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE\n");        \
      break;                                                    \
    case LIMINE_MEMMAP_ACPI_RECLAIMABLE:                        \
      klogi(                                                    \
        "Type: LIMINE_MEMMAP_ACPI_RECLAIMABLE\n");              \
      break;                                                    \
  }                                                             \
}

#define PHYS_TO_VIRT(in) (((uint64_t) in) + MEM_VIRT_OFFSET)
#define VIRT_TO_PHYS(in) (((uint64_t) in) - MEM_VIRT_OFFSET)
#define NUM_PAGES(addr)  (((addr) + PAGE_SIZE - 1) / PAGE_SIZE)
#define PAGE_ALIGN(addr) (NUM_PAGES(addr) * PAGE_SIZE)

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void pm_init(LIMINE_MEM_REQ req);
static inline void bitmap_set(uint64_t address, uint64_t num_pages);
static inline BITMAP_STATUS bitmap_free(uint64_t address, uint64_t num_pages);
STATUS pm_free(uint64_t address, uint64_t num_pages);
STATUS pm_allocate(uint64_t address, uint64_t num_pages);

/* --------------------------- EXTERNALLY DEFINED --------------------------- */