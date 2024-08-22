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
#include <structs/address_space_str.h>
#include <structs/memory_map_str.h>

#include <common/vector.h>
#include <common/memory.h>
#include <common/lock.h>
#include <common/limine_typedefs.h>

#include <sys/asm.h>
#include <sys/cpu.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
#define PAGES_PER_BYTE      (8)
#define PAGE_SIZE           (4096)

/* Determed from the HHDM offset input from bootloader */
#define MEM_VIRT_OFFSET     (0xFFFF800000000000)
#define FREE                (1)
#define ALLOCATED           (0)

typedef uint8_t BITMAP_STATUS;

/* Virtual Memory Page Flags */
#define VM_PRESENT          (1 << 0)
#define VM_READ_WRITE       (1 << 1)
#define VM_USER             (1 << 2)
#define VM_WRITE_THROUGH    (1 << 3)
#define VM_CACHE_DISABLE    (1 << 4)
#define VM_PAT              (1 << 7)

/* Virtual Memory Modes */
#define VM_DEFAULT          (VM_PRESENT | VM_READ_WRITE)
#define VM_MMIO             (VM_DEFAULT | VM_CACHE_DISABLE | VM_WRITE_THROUGH)
#define VM_USERMODE         (VM_DEFAULT | VM_USER)

#define PAGE_TABLE_ENTRIES  (512)
#define DEFAULT_PAGES       (8)

/* -------------------------------- GLOBALS --------------------------------- */
extern ADDR_SPACE kernel_addr_space;

/* --------------------------------- MACROS --------------------------------- */
#define ENTRY_TYPE_CHECK(entry)                                 \
      (entry->type == LIMINE_MEMMAP_USABLE ||                   \
      entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE ||    \
      entry->type == LIMINE_MEMMAP_KERNEL_AND_MODULES ||        \
      entry->type == LIMINE_MEMMAP_ACPI_RECLAIMABLE)            \

#define PRINT_MEM_SIZE(size) {                                  \
  klogi("%d -- B\n%d -- KB\n"                                   \
                  "%d -- MB\n",                                 \
                  size, size / 1024, size / 1024 / 1024);       \
}

#define ENTRY_INFO(entry) {                                     \
  switch (entry->type) {                                        \
    case LIMINE_MEMMAP_USABLE:                                  \
      klogi("Type: LIMINE_MEMMAP_USABLE\n");                    \
      break;                                                    \
    case LIMINE_MEMMAP_RESERVED:                                \
      klogi("Type: LIMINE_MEMMAP_RESERVED\n");                  \
      break;                                                    \
    case LIMINE_MEMMAP_ACPI_RECLAIMABLE:                        \
      klogi(                                                    \
        "Type: LIMINE_MEMMAP_ACPI_RECLAIMABLE\n");              \
      break;                                                    \
    case LIMINE_MEMMAP_ACPI_NVS:                                \
        klogi(                                                  \
          "Type: LIMINE_MEMMAP_ACPI_NVS\n");                    \
        break;                                                  \
    case LIMINE_MEMMAP_BAD_MEMORY:                              \
        klogi(                                                  \
          "Type: LIMINE_MEMMAP_BAD_MEMORY\n");                  \
        break;                                                  \
    case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:                  \
      klogi(                                                    \
        "Type: LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE\n");        \
      break;                                                    \
    case LIMINE_MEMMAP_KERNEL_AND_MODULES:                      \
      klogi(                                                    \
        "Type: LIMINE_MEMMAP_KERNEL_AND_MODULES\n");            \
      break;                                                    \
    case LIMINE_MEMMAP_FRAMEBUFFER:                             \
      klogi(                                                    \
        "Type: LIMINE_MEMMAP_FRAMEBUFFER\n");                   \
      break;                                                    \
  }                                                             \
}

#define PRINT_MEM_ENTRY_INFO(entry) {                           \
  klogi("Memory entry at range %x - %x\n"                       \
        "\t(Length: %d (%d KB))\n\t",                           \
        entry->base, entry->base + entry->length,               \
        entry->length, entry->length / 1024);                   \
  ENTRY_INFO(entry)                                             \
}

#define PHYS_TO_VIRT(in)              (((uint64_t) (in)) + MEM_VIRT_OFFSET)
#define VIRT_TO_PHYS(in)              (((uint64_t) (in)) - MEM_VIRT_OFFSET)
#define NUM_PAGES(addr)               (((addr) + PAGE_SIZE - 1) / PAGE_SIZE)
#define PAGE_ALIGN(addr)              (NUM_PAGES(addr) * PAGE_SIZE)
#define MAKE_TABLE_ENTRY(addr, flags) ((addr & ~(0xFFF)) | flags)

#define CHECK_NOT_PRESENT(entry)      (!((entry) & VM_PRESENT))
#define CHECK_PRESENT(entry)          (((entry) & VM_PRESENT))

#define CONVERT_ADDR_SPACE(as)        (!as ? &kernel_addr_space : as);

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void pm_init(LIMINE_MEM_REQ req);
STATUS pm_free(uint64_t address, uint64_t num_pages);
STATUS pm_allocate(uint64_t address, uint64_t num_pages);
uint64_t pm_get(uint64_t num_pages, uint64_t address, const char *func,
                size_t line_number);
void pm_used();
uint64_t vm_get_phys_addr(ADDR_SPACE *addr_space, uint64_t virt_addr);
void vm_unmap(ADDR_SPACE *addr_space, uint64_t virt_addr, uint64_t num_pages);
void vm_map(ADDR_SPACE *addr_space, uint64_t virt_addr, uint64_t phys_addr,
            uint64_t num_pages, uint64_t flags);
void vm_init(LIMINE_MEM_REQ req, LIMINE_K_ADDR_REQ k_req);
ADDR_SPACE *create_address_space();
