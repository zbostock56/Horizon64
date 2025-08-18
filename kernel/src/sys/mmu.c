/**
 * @file mmu.c
 * @author Zack Bostock
 * @brief Memory Management Unit
 * @verbatim
 * PML4 - Page Map Level 4
 * PDPT - Page Directory Pointer Table
 * PD - Paging Directory
 * PT - Page Table
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <sys/mmu.h>
#include <common/lock.h>
#include <common/math.h>

#if KMEM_DEBUG
#include <common/kmalloc.h>
#include <stdatomic.h>
#endif

static KERNEL_MEM_INFO kmem = {0};
vector_new_static(MEM_MAP, global_mem_map);
ADDR_SPACE kernel_addr_space = {0};
static LOCK vmm_lock = {0};

#define LOW_MEMORY_THRESHOLD    0x100000UL  /* 1 MB */
#define BITMAP_ALIGNMENT        PAGE_SIZE
#define MAX_DEBUG_PAGES         (1024 * 256)
#define BYTES_PER_MB            (1024 * 1024)

/* MMIO region definitions for common hardware */
#define APIC_BASE_ADDR          0xFEE00000UL
#define APIC_SIZE               0x1000UL
#define IOAPIC_BASE_ADDR        0xFEC00000UL
#define IOAPIC_SIZE             0x1000UL
#define PCI_CONFIG_BASE         0xE0000000UL
#define PCI_CONFIG_SIZE         0x10000000UL
#define HPET_BASE_ADDR          0xFED00000UL
#define HPET_SIZE               0x1000UL

/* X macro defined in mmu_str.h */
static const char *MMU_STATUS_STRINGS[] = {
#define X(name, str) str,
    MMU_STATUS_LIST
#undef X
};

/**
 * @brief Helper for checking the status of a mmu function
 *
 * @param status Status to check
 * @return uint8_t SYS_OK if okay, SYS_ERR if fail
 */
static STATUS check_status(MMU_STATUS status) {
    switch (status) {
    case MMU_SUCCESS:
        return SYS_OK;
    case MMU_ERR_NULL_POINTER:
    case MMU_ERR_NOT_MAPPED:
    case MMU_ERR_INVALID_ADDRESS:
    case MMU_ERR_INVALID_REQUEST:
    case MMU_ERR_OUT_OF_MEMORY:
    case MMU_ERR_DOUBLE_FREE:
        kloge("MMU: status: %s\n", MMU_STATUS_STRINGS[status]);
        return SYS_ERR;
    default:
        kloge("MMU: check_status: unknown status (%d)!\n", status);
        return SYS_ERR;
    }
}

/**
 * @brief Validates memory address range for safety
 * @param address Starting address
 * @param num_pages Number of pages
 * @return MMU_STATUS Error code
 */
static MMU_STATUS validate_address_range(uint64_t address, uint64_t num_pages) {
    if (num_pages == 0) {
        return MMU_ERR_INVALID_REQUEST;
    }
    
    /* Check for overflow */
    if (address > UINT64_MAX - (num_pages * PAGE_SIZE)) {
        return MMU_ERR_INVALID_ADDRESS;
    }
    
    /* Ensure address is page-aligned */
    if (address & (PAGE_SIZE - 1)) {
        return MMU_ERR_INVALID_ADDRESS;
    }
    
    return MMU_SUCCESS;
}

/**
 * @brief Memory statitics logging
 */
static void log_memory_statistics() {
    const uint64_t mb_divisor = BYTES_PER_MB;
    klogi("=== Memory Statistics ===\n");
    klogi("Total Memory: %d MB (%d bytes)\n", 
          kmem.total_size / mb_divisor, kmem.total_size);
    klogi("Physical Limit: %x\n", (void*)kmem.physical_limit);
    klogi("Virtual Base: %x\n", (void*)(kmem.physical_limit + MEM_VIRT_OFFSET));
    klogi("Free Memory: %d MB (%d bytes)\n", 
          kmem.free_size / mb_divisor, kmem.free_size);
    klogi("Used Memory: %d MB (%d bytes)\n",
          (kmem.total_size - kmem.free_size) / mb_divisor,
          kmem.total_size - kmem.free_size);
    klogi("========================\n");
}

/**
 * @brief Check bootloader memory map for additional MMIO regions
 * @param phys_addr Starting physical address
 * @param size Size in bytes
 * @return STATUS SYS_OK if found in bootloader memory map as valid MMIO
 */
static STATUS check_bootloader_mmio_regions(uint64_t phys_addr, uint64_t size) {
    /* This would check against the original Limine memory map for regions marked as: */
    /* - LIMINE_MEMMAP_ACPI_NVS */
    /* - LIMINE_MEMMAP_ACPI_RECLAIMABLE */
    /* - LIMINE_MEMMAP_RESERVED */
    /* - Any other MMIO regions reported by the bootloader */

    /* For now, just allow everything over 3GB */
    (void) size;
    if (phys_addr >= 0xC0000000UL) {
        return SYS_OK;
    }

    return SYS_ERR;
}

/**
 * @brief Check if a physical address range corresponds to valid MMIO
 * @param phys_addr Starting physical address
 * @param num_pages Number of pages
 * @return STATUS TRUE if this is a valid MMIO region
 */
static STATUS is_valid_mmio_region(uint64_t phys_addr, uint64_t num_pages) {
    uint64_t size = num_pages * PAGE_SIZE;
    uint64_t end_addr = phys_addr + size;

    /* Local APIC registers */
    if (phys_addr >= APIC_BASE_ADDR && end_addr <= APIC_BASE_ADDR + APIC_SIZE) {
        return TRUE;
    }

    /* I/O APIC registers */
    if (phys_addr >= IOAPIC_BASE_ADDR && end_addr <= IOAPIC_BASE_ADDR + IOAPIC_SIZE) {
        return TRUE;
    }

    /* HPET registers */
    if (phys_addr >= HPET_BASE_ADDR && end_addr <= HPET_BASE_ADDR + HPET_SIZE) {
        return TRUE;
    }

    /* PCI Express configuration space */
    if (phys_addr >= PCI_CONFIG_BASE && end_addr <= PCI_CONFIG_BASE + PCI_CONFIG_SIZE) {
        return TRUE;
    }

    /* VGA framebuffer region (0xA0000-0xBFFFF) */
    if (phys_addr >= 0xA0000 && end_addr <= 0xC0000) {
        return TRUE;
    }

    /* ROM/BIOS regions (0xC0000-0xFFFFF) */
    if (phys_addr >= 0xC0000 && end_addr <= 0x100000) {
        return TRUE;
    }

    /* Check against bootloader-provided memory map for additional MMIO regions */
    return check_bootloader_mmio_regions(phys_addr, size);
}

/**
 * @brief Thread-safe bitmap manipulation with bounds checking
 * @param address Starting address
 * @param num_pages Number of pages
 * @param allocate TRUE to allocate, FALSE to free
 * @return MMU_STATUS Error code
 */
static MMU_STATUS bitmap_modify(uint64_t address, uint64_t num_pages, uint8_t allocate) {
    MMU_STATUS result = validate_address_range(address, num_pages);
    if (result != MMU_SUCCESS) {
        return result;
    }
    
    if (address + (num_pages * PAGE_SIZE) > kmem.physical_limit) {
        return MMU_ERR_INVALID_ADDRESS;
    }
    
    uint64_t end = address + (num_pages * PAGE_SIZE);
    for (uint64_t i = address; i < end; i += PAGE_SIZE) {
        uint64_t page_idx = i / PAGE_SIZE;
        uint64_t byte_idx = page_idx / PAGES_PER_BYTE;
        uint8_t bit_idx = page_idx % PAGES_PER_BYTE;
        uint8_t bit_mask = 1 << bit_idx;
        
        if (allocate) {
            /* Check if already allocated */
            if (!(kmem.bitmap[byte_idx] & bit_mask)) {
                /* It is already allocated */
                return MMU_ERR_DOUBLE_FREE;
            }
            kmem.bitmap[byte_idx] &= ~bit_mask;
            kmem.free_size -= PAGE_SIZE;
        } else {
            /* Check for double free */
            if (kmem.bitmap[byte_idx] & bit_mask) {
                klogw("Double free detected at address %x\n", (void*)i);
                return MMU_ERR_DOUBLE_FREE;
            }
            kmem.bitmap[byte_idx] |= bit_mask;
            kmem.free_size += PAGE_SIZE;
        }
    }
    
    return MMU_SUCCESS;
}

/**
 * @brief Bitmap status check
 * @param address Starting address
 * @param num_pages Number of pages to check
 * @return BITMAP_STATUS Status of the pages
 */
static BITMAP_STATUS bitmap_check_status(uint64_t address, uint64_t num_pages) {
    if (validate_address_range(address, num_pages) != MMU_SUCCESS) {
        return ALLOCATED;
    }
    
    if (address + (num_pages * PAGE_SIZE) > kmem.physical_limit) {
        return ALLOCATED;
    }
    
    uint64_t end = address + (num_pages * PAGE_SIZE);
    for (uint64_t i = address; i < end; i += PAGE_SIZE) {
        uint64_t page_idx = i / PAGE_SIZE;
        uint64_t byte_idx = page_idx / PAGES_PER_BYTE;
        uint8_t bit_idx = page_idx % PAGES_PER_BYTE;
        uint8_t bit_mask = 1 << bit_idx;
        
        if (!(kmem.bitmap[byte_idx] & bit_mask)) {
            return ALLOCATED;
        }
    }
    return FREE;
}

/**
 * @brief Table allocation
 * @param table_entry Pointer to the table entry
 * @param addr_space The current address space structure
 * @param table_type Table type for debugging
 * @return uint64_t* Pointer to the allocated table, or NULL on failure
 */
static uint64_t *alloc_table_safe(uint64_t *table_entry, ADDR_SPACE *addr_space,
                                  int table_type) {
    if (!table_entry || !addr_space) {
        kloge("VM: Invalid parameters to alloc_table_safe\n");
        return NULL;
    }
    
    uint64_t entry_val = *table_entry;
    if (__builtin_expect(CHECK_NOT_PRESENT(entry_val), 0)) {
        uint64_t phys_addr = pm_get(DEFAULT_PAGES, 0x0, __func__, __LINE__);
        if (__builtin_expect(!phys_addr, 0)) {
            kloge("VM: Out of memory for table type %d for PML4 %x\n", 
                  table_type, addr_space->pml4);
            return NULL;
        }
        
        uint64_t *table = (uint64_t *)PHYS_TO_VIRT(phys_addr);
        memset(table, 0, PAGE_SIZE * DEFAULT_PAGES);
        
        *table_entry = MAKE_TABLE_ENTRY(phys_addr, VM_USERMODE);
        
        if (vector_append(&addr_space->memory_list, phys_addr) != 0) {
            kloge("VM: Failed to track allocated table in memory list\n");
            pm_free(phys_addr, DEFAULT_PAGES);
            return NULL;
        }
        
        return table;
    }
    return (uint64_t *)PHYS_TO_VIRT(entry_val & ~(0xFFF));
}

/**
 * @brief Page mapping
 * @param address_space Address space to map into
 * @param virt_addr Virtual address for the mapping
 * @param phys_addr Physical address for the mapping
 * @param flags Mapping flags
 * @return MMU_STATUS Error code
 */
static MMU_STATUS map_page_entry_safe(ADDR_SPACE *address_space, uint64_t virt_addr,
                                       uint64_t phys_addr, uint64_t flags) {
    if (!address_space) {
        return MMU_ERR_NULL_POINTER;
    }
    
    /* Validate addresses are page-aligned */
    if ((virt_addr & (PAGE_SIZE - 1)) || (phys_addr & (PAGE_SIZE - 1))) {
        return MMU_ERR_INVALID_ADDRESS;
    }
    
    ADDR_SPACE *addr_space = CONVERT_ADDR_SPACE(address_space);
    
    /* Extract page table indices */
    uint16_t pte   = (virt_addr >> 12) & 0x1FF;
    uint16_t pde   = (virt_addr >> 21) & 0x1FF;
    uint16_t pdpe  = (virt_addr >> 30) & 0x1FF;
    uint16_t pml4e = (virt_addr >> 39) & 0x1FF;

    uint64_t *pml4 = addr_space->pml4;
    if (!pml4) {
        return MMU_ERR_NULL_POINTER;
    }

    /* Allocate page tables, if needed */
    uint64_t *pdpt = alloc_table_safe(&pml4[pml4e], addr_space, 1);
    if (!pdpt) return MMU_ERR_OUT_OF_MEMORY;
    
    uint64_t *pd = alloc_table_safe(&pdpt[pdpe], addr_space, 2);
    if (!pd) return MMU_ERR_OUT_OF_MEMORY;
    
    uint64_t *pt = alloc_table_safe(&pd[pde], addr_space, 3);
    if (!pt) return MMU_ERR_OUT_OF_MEMORY;

    /* Check if page is already mapped */
    if (!CHECK_NOT_PRESENT(pt[pte])) {
        klogw("VM: Page already mapped at virt=%x, overwriting\n", (void*)virt_addr);
    }

    /* Map physical address into page table entry */
    pt[pte] = MAKE_TABLE_ENTRY(phys_addr & ~(0xFFF), flags);

    /* Invalidate TLB if address space is active */
    if (addr_space->is_init && 
        __builtin_expect(read_cr(cr3) == VIRT_TO_PHYS(addr_space->pml4), 1)) {
        __asm__ volatile ("invlpg (%0)" : : "r"(virt_addr) : "memory");
    }
    
    return MMU_SUCCESS;
}

/**
 * @brief Physical memory initialization
 * @param req Request from Limine bootloader
 * @return MMU_STATUS Success or error code
 */
MMU_STATUS pm_init(LIMINE_MEM_REQ req) {
    klogs("INIT PM: starting...\n");
    
    LIMINE_MEM_RES *res = req.response;
    if (!res || !res->entry_count) {
        kloge("PM INIT: Memory map request unfulfilled!\n");
        return MMU_ERR_NULL_POINTER;
    }

    kmem.free_size = 0;
    kmem.physical_limit = 0;
    kmem.total_size = 0;
    kmem.bitmap = NULL;

    uint64_t entry_count = res->entry_count;
    
    /* First pass: calculator total_size and physical_limit */
    for (uint64_t i = 0; i < entry_count; i++) {
        struct limine_memmap_entry *entry = res->entries[i];
        if (!ENTRY_TYPE_CHECK(entry)) {
            continue;
        }
        
        if (kmem.total_size > UINT64_MAX - entry->length) {
            kloge("PM INIT: Memory size overflow detected!\n");
            return MMU_ERR_INVALID_ADDRESS;
        }
        
        kmem.total_size += entry->length;
        uint64_t new_limit = entry->base + entry->length;
        if (new_limit > kmem.physical_limit) {
            kmem.physical_limit = new_limit;
        }
    }

    /* Calculate bitmap size with proper alignment */
    uint64_t bitmap_size = ALIGNUP(kmem.physical_limit / (PAGE_SIZE * PAGES_PER_BYTE), 
                                   BITMAP_ALIGNMENT);
    
    /* Find suitable location for bitmap */
    uint8_t bitmap_located = FALSE;
    for (uint64_t i = 0; i < entry_count; i++) {
        struct limine_memmap_entry *entry = res->entries[i];
        
        /* Skip low memory region */
        if (entry->base + entry->length <= LOW_MEMORY_THRESHOLD) {
            continue;
        }
        
        if (entry->length >= bitmap_size && entry->type == LIMINE_MEMMAP_USABLE) {
            kmem.bitmap = (uint8_t *)PHYS_TO_VIRT(entry->base);
            bitmap_located = TRUE;
            break;
        }
    }
    
    if (!bitmap_located) {
        kloge("PM INIT: Could not find suitable location for bitmap!\n");
        return MMU_ERR_OUT_OF_MEMORY;
    }

    /* Init bitmap with all pages marked as allocated to start */
    memset(kmem.bitmap, 0, bitmap_size);
    klogi("Physical Memory Bitmap Location: %x (size: %d bytes)\n", 
          kmem.bitmap, bitmap_size);

    /* Second pass: mark usable pages as free */
    for (uint64_t i = 0; i < entry_count; i++) {
        struct limine_memmap_entry *entry = res->entries[i];
        if (entry->base + entry->length <= LOW_MEMORY_THRESHOLD) {
            continue;
        }
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            MMU_STATUS result = pm_free(entry->base, NUM_PAGES(entry->length));
            if (check_status(result) != SYS_OK) {
                klogw("PM INIT: Failed to mark region as free: base=%x, length=%d\n",
                      (void*)entry->base, entry->length);
            }
        }
    }

    /* Reserve bitmap memory */
    MMU_STATUS result = pm_allocate(VIRT_TO_PHYS(kmem.bitmap),
                                                 NUM_PAGES(bitmap_size));
    if (check_status(result) != SYS_OK) {
        kloge("PM INIT: Failed to reserve bitmap memory!\n");
        return result;
    }
    
    log_memory_statistics();
    klogs("INIT PM: finished successfully\n");
    return MMU_SUCCESS;
}

#if KMEM_DEBUG
/**
 * @brief  memory debugging with atomic counters
 */
void mem_debug() {
    static atomic_int debug_call_count = 0;
    int call_num = atomic_fetch_add(&debug_call_count, 1);
    
    klogi("=== MEMORY DEBUG START (Call #%d) ===\n", call_num);
    klogi("Current checkno: %d\n", kmalloc_checkno);

    const uint64_t max_pages = MIN(NUM_PAGES(kmem.physical_limit), MAX_DEBUG_PAGES);
    const uint64_t end_addr = max_pages * PAGE_SIZE;
    
    uint64_t total_debug_allocations = 0;
    uint64_t total_debug_size = 0;

    for (uint64_t addr = 0; addr < end_addr; addr += PAGE_SIZE) {
        if (bitmap_check_status(addr, 1) == FREE) {
            continue;
        }

        KMEM_METADATA *meta = (KMEM_METADATA *)PHYS_TO_VIRT(addr);
        
        if (meta->magic != KMEM_MAGIC_NUMBER) {
            continue;
        }

        if (meta->checkno == kmalloc_checkno && kmalloc_checkno > 0) {
            klogi("  %x %s():%d - %d bytes (%d MB)\n",
                  (void *)meta,
                  meta->file_name ? meta->file_name : "unknown",
                  meta->lineno,
                  meta->size,
                  meta->size / BYTES_PER_MB);
            
            total_debug_allocations++;
            total_debug_size += meta->size;
        }
    }
    
    klogi("Total debug allocations: %d (%d MB)\n", 
          total_debug_allocations, total_debug_size / BYTES_PER_MB);
    klogi("Advancing checkno to: %d\n", ++kmalloc_checkno);
    klogi("=== MEMORY DEBUG END ===\n");
}
#endif

/**
 * @brief Memory usage reporting
 */
void pm_used() {
    log_memory_statistics();
    
#if KMEM_DEBUG
    mem_debug();
#endif
}

/**
 * @brief Page freeing
 * @param address Base address to free from
 * @param num_pages Number of physical pages to free
 * @return MMU_STATUS Error code
 */
MMU_STATUS pm_free(uint64_t address, uint64_t num_pages) {
    if (!kmem.bitmap) {
        return MMU_ERR_NULL_POINTER;
    }
    
    return bitmap_modify(address, num_pages, FALSE);
}

/**
 * @brief Page allocation
 * @param address Base address to allocate from
 * @param num_pages Number of physical pages to allocate
 * @return MMU_STATUS Error code
 */
MMU_STATUS pm_allocate(uint64_t address, uint64_t num_pages) {
    if (!kmem.bitmap) {
        return MMU_ERR_NULL_POINTER;
    }
    
    /* Check if pages are available */
    if (bitmap_check_status(address, num_pages) != FREE) {
        return MMU_ERR_OUT_OF_MEMORY;
    }
    
    return bitmap_modify(address, num_pages, TRUE);
}

/**
 * @brief Physical page allocation
 * @param num_pages Number of pages required
 * @param start_address Starting search address (0 for any)
 * @param func Function calling pm_get
 * @param line_number Line number in the caller
 * @return uint64_t Starting physical address of allocated pages, or 0 on failure
 */
uint64_t pm_get(uint64_t num_pages, uint64_t start_address, 
                const char *func, size_t line_number) {
    
    if (num_pages == 0) {
        klogw("pm_get: %s:%d requested 0 pages\n", func, line_number);
        return 0;
    }
    
    /* Ensure start address is page-aligned */
    start_address = ALIGNUP(start_address, PAGE_SIZE);
    
    /* Check if we have enough free memory */
    if (kmem.free_size < num_pages * PAGE_SIZE) {
        kloge("pm_get: %s:%d insufficient memory: requested %d pages (%d bytes), available %d bytes\n",
              func, line_number, num_pages, num_pages * PAGE_SIZE, kmem.free_size);
        halt();
        return 0;
    }
    
    /* Search for contiguous free pages */
    for (uint64_t i = start_address; i < kmem.physical_limit; i += PAGE_SIZE) {
        if (i + (num_pages * PAGE_SIZE) > kmem.physical_limit) {
            break;
        }
        
        if (pm_allocate(i, num_pages) == MMU_SUCCESS) {
            klogd("pm_get: %s:%d allocated %d pages at %x\n", 
                  func, line_number, num_pages, (void*)i);
            return i;
        }
    }
    
    kloge("pm_get: %s:%d failed to find %d contiguous pages (free: %d bytes)\n",
          func, line_number, num_pages, kmem.free_size);
    halt();
    return 0;
}

/**
 * @brief Virtual to physical address translation
 * @param addr_space Address space to search in (NULL for kernel space)
 * @param virt_addr Virtual address to convert
 * @param phys_addr_out Pointer to store the physical address result
 * @return MMU_STATUS Error code (MMU_SUCCESS if mapped, MMU_ERR_NOT_MAPPED if not)
 */
static MMU_STATUS vm_get_phys_addr_impl(ADDR_SPACE *addr_space, uint64_t virt_addr, 
                                  uint64_t *phys_addr_out) {
    if (!phys_addr_out) {
        return MMU_ERR_NULL_POINTER;
    }
    
    *phys_addr_out = 0;
    
    /* Use kernel address space if non specified */
    ADDR_SPACE *as = CONVERT_ADDR_SPACE(addr_space);
    if (!as || !as->pml4) {
        return MMU_ERR_NULL_POINTER;
    }
    
    /* Extract page table indices */
    uint16_t pte   = (virt_addr >> 12) & 0x1FF;
    uint16_t pde   = (virt_addr >> 21) & 0x1FF;
    uint16_t pdpe  = (virt_addr >> 30) & 0x1FF;
    uint16_t pml4e = (virt_addr >> 39) & 0x1FF;

    uint64_t *pml4 = as->pml4;
    
    /* Check PML4 entry */
    if (CHECK_NOT_PRESENT(pml4[pml4e])) {
        return MMU_ERR_NOT_MAPPED;
    }
    
    /* Check PDPT entry */
    uint64_t *pdpt = (uint64_t *)PHYS_TO_VIRT(pml4[pml4e] & ~(0xFFF));
    if (CHECK_NOT_PRESENT(pdpt[pdpe])) {
        return MMU_ERR_NOT_MAPPED;
    }
    
    /* Check PD entry */
    uint64_t *pd = (uint64_t *)PHYS_TO_VIRT(pdpt[pdpe] & ~(0xFFF));
    if (CHECK_NOT_PRESENT(pd[pde])) {
        return MMU_ERR_NOT_MAPPED;
    }
    
    /* Check PT entry */
    uint64_t *pt = (uint64_t *)PHYS_TO_VIRT(pd[pde] & ~(0xFFF));
    if (CHECK_NOT_PRESENT(pt[pte])) {
        return MMU_ERR_NOT_MAPPED;
    }
    
    /* Extract physical address and preserve page offset */
    *phys_addr_out = (pt[pte] & 0xFFFFFFFFFFFFF000) | (virt_addr & 0xFFF);
    return MMU_SUCCESS;
}

/**
 * @brief Helper to remove an address from the memory tracking list
 * @param as Address space
 * @param phys_addr Physical address to remove
 */
static void remove_from_memory_list(ADDR_SPACE *as, uint64_t phys_addr) {
    if (!as) return;
    
    size_t len = vector_len(&as->memory_list);
    for (size_t i = 0; i < len; i++) {
        if (vector_at(&as->memory_list, i) == phys_addr) {
            vector_erase(&as->memory_list, i);
            break;
        }
    }
}

/**
 * @brief Helper function to unmap a single page with table cleanup
 * @param as Address space to unmap from
 * @param virt_addr Virtual address of the page
 * @return MMU_STATUS Error code
 */
static MMU_STATUS unmap_single_page(ADDR_SPACE *as, uint64_t virt_addr) {
    if (!as || !as->pml4) {
        return MMU_ERR_NULL_POINTER;
    }
    
    uint16_t pte   = (virt_addr >> 12) & 0x1FF;
    uint16_t pde   = (virt_addr >> 21) & 0x1FF;
    uint16_t pdpe  = (virt_addr >> 30) & 0x1FF;
    uint16_t pml4e = (virt_addr >> 39) & 0x1FF;

    uint64_t *pml4 = as->pml4;
    
    /* Check if PML4 entry exists */
    if (CHECK_NOT_PRESENT(pml4[pml4e])) {
        return MMU_ERR_NOT_MAPPED;
    }

    uint64_t *pdpt = (uint64_t *)PHYS_TO_VIRT(pml4[pml4e] & ~(0xFFF));
    if (CHECK_NOT_PRESENT(pdpt[pdpe])) {
        return MMU_ERR_NOT_MAPPED;
    }

    uint64_t *pd = (uint64_t *)PHYS_TO_VIRT(pdpt[pdpe] & ~(0xFFF));
    if (CHECK_NOT_PRESENT(pd[pde])) {
        return MMU_ERR_NOT_MAPPED;
    }

    uint64_t *pt = (uint64_t *)PHYS_TO_VIRT(pd[pde] & ~(0xFFF));
    if (CHECK_NOT_PRESENT(pt[pte])) {
        return MMU_ERR_NOT_MAPPED;
    }

    /* Unmap the page */
    pt[pte] = 0;

    /* Invalidate TLB if this address space is active */
    if (as->is_init && read_cr(cr3) == VIRT_TO_PHYS(as->pml4)) {
        __asm__ volatile ("invlpg (%0)" : : "r"(virt_addr) : "memory");
    }

    /* Check if we can clean up the page table */
    uint8_t pt_empty = TRUE;
    for (size_t i = 0; i < 512; i++) {
        if (pt[i] != 0) {
            pt_empty = FALSE;
            break;
        }
    }

    if (pt_empty) {
        /* Free the page table */
        pd[pde] = 0;
        MMU_STATUS free_result = pm_free(VIRT_TO_PHYS(pt), DEFAULT_PAGES);
        if (check_status(free_result) != SYS_OK) {
            klogw("VM: Failed to free page table at %x\n", pt);
        }
        
        /* Remove from memory tracking list */
        remove_from_memory_list(as, VIRT_TO_PHYS(pt));

        /* Check if we can clean up the page directory */
        uint8_t pd_empty = TRUE;
        for (size_t i = 0; i < 512; i++) {
            if (pd[i] != 0) {
                pd_empty = FALSE;
                break;
            }
        }

        if (pd_empty) {
            /* Free the page directory */
            pdpt[pdpe] = 0;
            free_result = pm_free(VIRT_TO_PHYS(pd), DEFAULT_PAGES);
            if (check_status(free_result) != SYS_OK) {
                klogw("VM: Failed to free page directory at %x\n", pd);
            }
            
            remove_from_memory_list(as, VIRT_TO_PHYS(pd));

            /* Check if we can clean up the PDPT */
            uint8_t pdpt_empty = TRUE;
            for (size_t i = 0; i < 512; i++) {
                if (pdpt[i] != 0) {
                    pdpt_empty = FALSE;
                    break;
                }
            }

            if (pdpt_empty) {
                /* Free from PDPT */
                pml4[pml4e] = 0;
                free_result = pm_free(VIRT_TO_PHYS(pdpt), DEFAULT_PAGES);
                if (check_status(free_result) != SYS_OK) {
                    klogw("VM: Failed to free PDPT at %x\n", pdpt);
                }
                
                remove_from_memory_list(as, VIRT_TO_PHYS(pdpt));
            }
        }
    }

    return MMU_SUCCESS;
}

/**
 * @brief Virtual memory initialization
 * @param req Memory map request
 * @param k_req Kernel address request
 * @return MMU_STATUS Error code
 */
MMU_STATUS vm_init(LIMINE_MEM_REQ req, LIMINE_K_ADDR_REQ k_req) {
    klogs("INIT VM: starting...\n");
    
    LIMINE_MEM_RES *m = req.response;
    LIMINE_K_ADDR_RES *kernel = k_req.response;
    
    if (!m) {
        kloge("INIT VM: Memory map request is NULL!\n");
        return MMU_ERR_NULL_POINTER;
    }
    
    if (!kernel) {
        kloge("INIT VM: Kernel address request is NULL!\n");
        return MMU_ERR_NULL_POINTER;
    }

    /* Allocate kernel PML4 */
    uint64_t pml4_phys = pm_get(DEFAULT_PAGES, 0x0, __func__, __LINE__);
    if (!pml4_phys) {
        kloge("INIT VM: Failed to allocate kernel PML4!\n");
        return MMU_ERR_OUT_OF_MEMORY;
    }
    
    kernel_addr_space.pml4 = (void *)PHYS_TO_VIRT(pml4_phys);
    memset(kernel_addr_space.pml4, 0, PAGE_SIZE * DEFAULT_PAGES);

    /* Map all physical memory to higher half */
    size_t num_pages = NUM_PAGES(kmem.physical_limit);
    klogi("Mapping %d pages (%d MB) to higher half at %x\n",
          num_pages, (num_pages * PAGE_SIZE) / BYTES_PER_MB, 
          (void*)MEM_VIRT_OFFSET);
    
    for (uint64_t i = 0; i < num_pages * PAGE_SIZE; i += PAGE_SIZE) {
        MMU_STATUS result = map_page_entry_safe(NULL, MEM_VIRT_OFFSET + i,
                                                i, VM_DEFAULT);
        if (check_status(result) != SYS_OK) {
            kloge("INIT VM: Failed to map physical page at %x\n", (void*)i);
            return result;
        }
    }

    /* Map specific regions from memory map */
    MMU_STATUS result = MMU_SUCCESS;
    for (size_t i = 0; i < m->entry_count; i++) {
        struct limine_memmap_entry *entry = m->entries[i];
        
        switch (entry->type) {
            case LIMINE_MEMMAP_KERNEL_AND_MODULES: {
                uint64_t virt_addr = kernel->virtual_base + 
                                   entry->base - kernel->physical_base;
                vm_map(NULL, virt_addr, entry->base, 
                       NUM_PAGES(entry->length), VM_DEFAULT);
                klogd("Mapped kernel: phys=%x -> virt=%x (length=%d)\n",
                      (void*)entry->base, (void*)virt_addr, entry->length);
                break;
            }
            case LIMINE_MEMMAP_FRAMEBUFFER:
            case LIMINE_MEMMAP_USABLE:
            case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
            case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE: {
                vm_map(NULL, PHYS_TO_VIRT(entry->base), entry->base,
                       NUM_PAGES(entry->length), VM_DEFAULT);
                klogd("Mapped region type %d: phys=%x -> virt=%x (length=%d)\n",
                      entry->type, (void*)entry->base, 
                      (void*)PHYS_TO_VIRT(entry->base), entry->length);
                break;
            }
            default:
                klogd("Skipping region type %d at %x (length=%d)\n",
                      entry->type, (void*)entry->base, entry->length);
                break;
        }
    }

    kernel_addr_space.is_init = TRUE;
    write_cr(cr3, VIRT_TO_PHYS(kernel_addr_space.pml4));
    
    klogs("INIT VM: finished successfully\n");
    return result;
}

/**
 * @brief  Address space creation
 * @return ADDR_SPACE* Pointer to new address space, or NULL on failure
 */
ADDR_SPACE *create_address_space() {
    ADDR_SPACE *as = kmalloc(sizeof(ADDR_SPACE));
    if (!as) {
        kloge("VM: Cannot allocate address space structure!\n");
        return NULL;
    }

    LOCK_LOCK(&vmm_lock);
    memset(as, 0, sizeof(ADDR_SPACE));

    // Allocate PML4 for the new address space
    as->pml4 = kcmalloc(PAGE_SIZE * DEFAULT_PAGES);
    if (!as->pml4) {
        kloge("VM: Cannot allocate PML4 for new address space!\n");
        kfree(as);
        UNLOCK_LOCK(&vmm_lock);
        return NULL;
    }
    
    memset(as->pml4, 0, PAGE_SIZE * DEFAULT_PAGES);
    as->lock = LOCK_NEW;

    /* Copy global memory mappings */
    size_t len = vector_len(&global_mem_map);
    for (size_t i = 0; i < len; i++) {
        MEM_MAP map = vector_at(&global_mem_map, i);
        vm_map(as, map.virt_addr, map.phys_addr, map.num_pages, map.flags);
    }
    
    UNLOCK_LOCK(&vmm_lock);
    as->is_init = TRUE;
    
    klogd("VMM: Created address space at %x with %d global mappings\n", as, len);
    return as;
}

/**
 * @brief Check if a virtual address is mapped in an address space
 * @param addr_space Address space to check
 * @param virt_addr Virtual address to check
 * @return uint8_t TRUE if mapped, FALSE otherwise
 */
uint8_t vm_is_mapped(ADDR_SPACE *addr_space, uint64_t virt_addr) {
    return vm_get_phys_addr(addr_space, virt_addr) != 0;
}

/**
 * @brief Wrapper for vm_get_phys_addr_impl
 * @param addr_space Address space to search in
 * @param virt_addr Virtual address to convert
 * @return uint64_t Physical address, or 0 if not mapped
 */
uint64_t vm_get_phys_addr(ADDR_SPACE *addr_space, uint64_t virt_addr) {
    uint64_t phys_addr;
    if (vm_get_phys_addr_impl(addr_space, virt_addr, &phys_addr) == MMU_SUCCESS) {
        return phys_addr;
    }
    return 0;
}

/**
 * @brief Page unmapping
 * @param addr_space Address space to unmap from (NULL for global memory map)
 * @param virt_addr Virtual address of the first page to unmap
 * @param num_pages Number of pages to unmap
 * @return MMU_STATUS Error code
 */
MMU_STATUS vm_unmap_impl(ADDR_SPACE *addr_space, uint64_t virt_addr, 
                          uint64_t num_pages) {
    
    MMU_STATUS result = validate_address_range(virt_addr, num_pages);
    if (check_status(result) != SYS_OK) {
        return result;
    }
    
    /* Handle global memory map case */
    if (!addr_space) {
        LOCK_LOCK(&vmm_lock);
        size_t len = vector_len(&global_mem_map);
        uint8_t found = FALSE;
        
        for (size_t i = 0; i < len; i++) {
            MEM_MAP map = vector_at(&global_mem_map, i);
            if (map.virt_addr == virt_addr && map.num_pages >= num_pages) {
                if (map.num_pages == num_pages) {
                    /* Remove entire mapping */
                    vector_erase(&global_mem_map, i);
                } else {
                    /* Partial unmap - adjust the mapping */
                    map.virt_addr += num_pages * PAGE_SIZE;
                    map.phys_addr += num_pages * PAGE_SIZE;
                    map.num_pages -= num_pages;
                    vector_set(&global_mem_map, i, map);
                }
                found = TRUE;
                break;
            }
        }
        UNLOCK_LOCK(&vmm_lock);
        
        if (!found) {
            klogw("VM: Attempted to unmap non-existent global mapping at %x\n", 
                  (void*)virt_addr);
            return MMU_ERR_NOT_MAPPED;
        }
        return MMU_SUCCESS;
    }
    
    /* Handle address space specific unmapping */
    ADDR_SPACE *as = CONVERT_ADDR_SPACE(addr_space);
    if (!as || !as->pml4) {
        return MMU_ERR_NULL_POINTER;
    }
    
    LOCK_LOCK(&as->lock);
    
    uint64_t unmapped_count = 0;
    uint64_t end_addr = virt_addr + (num_pages * PAGE_SIZE);
    
    for (uint64_t addr = virt_addr; addr < end_addr; addr += PAGE_SIZE) {
        MMU_STATUS unmap_result = unmap_single_page(as, addr);
        if (check_status(unmap_result) == SYS_OK) {
            unmapped_count++;
        } else if (unmap_result != MMU_ERR_NOT_MAPPED) {
            /* Critical error during unmapping */
            UNLOCK_LOCK(&as->lock);
            kloge("VM: Critical error unmapping page at %x: error: %s\n",
                  (void*)addr, MMU_STATUS_STRINGS[unmap_result]);
            return unmap_result;
        }
    }
    
    UNLOCK_LOCK(&as->lock);
    
    if (unmapped_count == 0) {
        klogw("VM: No pages were unmapped in range %x-%x\n", 
              (void*)virt_addr, (void*)end_addr);
        return MMU_ERR_NOT_MAPPED;
    }
    
    klogd("VM: Successfully unmapped %d/%d pages starting at %x\n",
          unmapped_count, num_pages, (void*)virt_addr);
    
    return MMU_SUCCESS;
}

/**
 * @brief Wrapper for vm_unmap_impl
 * @param addr_space Address space to unmap from
 * @param virt_addr Virtual address to unmap
 * @param num_pages Number of pages to unmap
 */
void vm_unmap(ADDR_SPACE *addr_space, uint64_t virt_addr, uint64_t num_pages) {
    MMU_STATUS result = vm_unmap_impl(addr_space, virt_addr, num_pages);
    if (check_status(result) != SYS_OK) {
        klogw("VM: vm_unmap failed with error %d for address %x\n", 
              result, (void*)virt_addr);
    }
}

/**
 * @brief Virtual memory mapping
 * @param addr_space Address space to map into (NULL for global memory map)
 * @param virt_addr Virtual address for the mapping
 * @param phys_addr Physical address for the mapping  
 * @param num_pages Number of pages to map
 * @param flags Mapping flags
 * @return MMU_STATUS Error code
 */
MMU_STATUS vm_map_impl(ADDR_SPACE *addr_space, uint64_t virt_addr,
                        uint64_t phys_addr, uint64_t num_pages, uint64_t flags) {
    
    MMU_STATUS result = validate_address_range(virt_addr, num_pages);
    if (result != MMU_SUCCESS) {
        return result;
    }
    
    result = validate_address_range(phys_addr, num_pages);
    if (result != MMU_SUCCESS) {
        return result;
    }

    /* Validate physical addresses, but allow MMIO regions above RAM limit */
    if (phys_addr + (num_pages * PAGE_SIZE) > kmem.physical_limit) {
        if (!is_valid_mmio_region(phys_addr, num_pages)) {
            kloge("VM: Physical address range %x-%x exceeds system limit %x and is not valid MMIO\n",
                  phys_addr, (phys_addr + num_pages * PAGE_SIZE),
                  kmem.physical_limit);
            return MMU_ERR_INVALID_ADDRESS;
        } else {
            klogd("VM: Allowing MMIO mapping: phys=%x-%x (beyond RAM limit %x)\n",
                  phys_addr, (phys_addr + num_pages * PAGE_SIZE),
                  kmem.physical_limit);
        }
    }
    
    /* Handle global memory map case */
    if (!addr_space) {
        MEM_MAP new_map = {
            .virt_addr = virt_addr,
            .phys_addr = phys_addr,
            .flags = flags,
            .num_pages = num_pages
        };
        
        LOCK_LOCK(&vmm_lock);
        
        /* Check for overlapping mappings */
        size_t len = vector_len(&global_mem_map);
        for (size_t i = 0; i < len; i++) {
            MEM_MAP existing = vector_at(&global_mem_map, i);
            uint64_t existing_end = existing.virt_addr +
                                    (existing.num_pages * PAGE_SIZE);
            uint64_t new_end = virt_addr + (num_pages * PAGE_SIZE);
            
            /* Check for overlap */
            if (!(new_end <= existing.virt_addr || virt_addr >= existing_end)) {
                UNLOCK_LOCK(&vmm_lock);
                kloge("VM: Global mapping overlap detected: new[%x-%x] vs existing[%x-%x]\n",
                      (void*)virt_addr, (void*)new_end,
                      (void*)existing.virt_addr, (void*)existing_end);
                return MMU_ERR_INVALID_ADDRESS;
            }
        }
        
        if (vector_append(&global_mem_map, new_map) != 0) {
            UNLOCK_LOCK(&vmm_lock);
            kloge("VM: Failed to add mapping to global memory map\n");
            return MMU_ERR_OUT_OF_MEMORY;
        }
        
        UNLOCK_LOCK(&vmm_lock);
        
        klogd("VM: Added global mapping: virt=%x phys=%x pages=%d flags=%x\n",
              (void*)virt_addr, (void*)phys_addr, num_pages, flags);
        return MMU_SUCCESS;
    }
    
    /* Handle address space specific mapping */
    ADDR_SPACE *as = CONVERT_ADDR_SPACE(addr_space);
    if (!as || !as->pml4) {
        return MMU_ERR_NULL_POINTER;
    }
    
    LOCK_LOCK(&as->lock);
    
    uint64_t mapped_count = 0;
    uint64_t end_addr = virt_addr + (num_pages * PAGE_SIZE);
    
    /* Map each page individually */
    for (uint64_t vaddr = virt_addr, paddr = phys_addr; 
         vaddr < end_addr; 
         vaddr += PAGE_SIZE, paddr += PAGE_SIZE) {
         
        MMU_STATUS map_result = map_page_entry_safe(as, vaddr, paddr, flags);
        if (check_status(map_result) != SYS_OK) {
            /* Rollback already mapped pages if fail */
            if (mapped_count > 0) {
                klogw("VM: Rolling back %d successfully mapped pages due to error\n", 
                      mapped_count);
                for (uint64_t rollback_addr = virt_addr; 
                     rollback_addr < vaddr; 
                     rollback_addr += PAGE_SIZE) {
                    unmap_single_page(as, rollback_addr);
                }
            }
            
            UNLOCK_LOCK(&as->lock);
            kloge("VM: Failed to map page virt=%x phys=%x: error %d\n",
                  (void*)vaddr, (void*)paddr, map_result);
            return map_result;
        }
        mapped_count++;
    }
    
    UNLOCK_LOCK(&as->lock);
    
    klogd("VM: Successfully mapped %d pages: virt=%x phys=%x flags=%x\n",
          num_pages, (void*)virt_addr, (void*)phys_addr, flags);
    
    return MMU_SUCCESS;
}

/**
 * @brief Wrapper for vm_map_impl
 * @param addr_space Address space to map into
 * @param virt_addr Virtual address for mapping
 * @param phys_addr Physical address for mapping
 * @param num_pages Number of pages to map
 * @param flags Mapping flags
 */
void vm_map(ADDR_SPACE *addr_space, uint64_t virt_addr, uint64_t phys_addr, 
            uint64_t num_pages, uint64_t flags) {
    MMU_STATUS result = vm_map_impl(addr_space, virt_addr, phys_addr, num_pages, flags);
    if (check_status(result) != SYS_OK) {
        kloge("VM: vm_map failed with error %d for virt=%x phys=%x\n",
              result, (void*)virt_addr, (void*)phys_addr);
        if (result == MMU_ERR_OUT_OF_MEMORY) {
            halt();
        }
    }
}

/**
 * @brief Check mapping permissions for a virtual address
 * @param addr_space Address space to check
 * @param virt_addr Virtual address to check
 * @param flags_out Pointer to store the mapping flags
 * @return MMU_STATUS Error code
 */
MMU_STATUS vm_get_mapping_flags(ADDR_SPACE *addr_space, uint64_t virt_addr, 
                                 uint64_t *flags_out) {
    if (!flags_out) {
        return MMU_ERR_NULL_POINTER;
    }
    
    ADDR_SPACE *as = CONVERT_ADDR_SPACE(addr_space);
    if (!as || !as->pml4) {
        return MMU_ERR_NULL_POINTER;
    }
    
    uint16_t pte   = (virt_addr >> 12) & 0x1FF;
    uint16_t pde   = (virt_addr >> 21) & 0x1FF;
    uint16_t pdpe  = (virt_addr >> 30) & 0x1FF;
    uint16_t pml4e = (virt_addr >> 39) & 0x1FF;

    uint64_t *pml4 = as->pml4;
    
    if (CHECK_NOT_PRESENT(pml4[pml4e])) return MMU_ERR_NOT_MAPPED;
    uint64_t *pdpt = (uint64_t *)PHYS_TO_VIRT(pml4[pml4e] & ~(0xFFF));
    
    if (CHECK_NOT_PRESENT(pdpt[pdpe])) return MMU_ERR_NOT_MAPPED;
    uint64_t *pd = (uint64_t *)PHYS_TO_VIRT(pdpt[pdpe] & ~(0xFFF));
    
    if (CHECK_NOT_PRESENT(pd[pde])) return MMU_ERR_NOT_MAPPED;
    uint64_t *pt = (uint64_t *)PHYS_TO_VIRT(pd[pde] & ~(0xFFF));
    
    if (CHECK_NOT_PRESENT(pt[pte])) return MMU_ERR_NOT_MAPPED;
    
    /* Extract flags from lower 12 bits */
    *flags_out = pt[pte] & 0xFFF;
    return MMU_SUCCESS;
}