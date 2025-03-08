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
 * @copyright Copyright (c) 2024
 *
 */

#include <sys/mmu.h>
#include <common/lock.h>

static KERNEL_MEM_INFO kmem = {0};
vector_new_static(MEM_MAP, global_mem_map);
ADDR_SPACE kernel_addr_space = {0};
static LOCK vmm_lock = {0};

/**
 * @brief Physical memory initialization
 *
 * @param req Request from Limine bootloader
 */
void pm_init(LIMINE_MEM_REQ req) {
    klogs("INIT PM: starting...\n");
    LIMINE_MEM_RES *res = req.response;
    if (!res || !res->entry_count) {
        kloge("PM INIT: Memory map request unfulfilled!\n");
        halt();
    }

    kmem.free_size = 0;
    kmem.physical_limit = 0;
    kmem.total_size = 0;

    uint64_t entry_count = res->entry_count;
    /* First pass: calculate total_size and physical_limit. */
    for (uint64_t i = 0; i < entry_count; i++) {
        struct limine_memmap_entry *entry = res->entries[i];
        if (!ENTRY_TYPE_CHECK(entry)) {
            continue;
        }
        kmem.total_size += entry->length;
        uint64_t new_limit = entry->base + entry->length;
        if (new_limit > kmem.physical_limit) {
            kmem.physical_limit = new_limit;
        }
    }

    uint64_t bitmap_size = kmem.physical_limit /
      (PAGE_SIZE * PAGES_PER_BYTE);
    /* Choose a good location for the bitmap. */
    for (uint64_t i = 0; i < entry_count; i++) {
        struct limine_memmap_entry *entry = res->entries[i];
        if (entry->base + entry->length <= 0x100000) {
            continue;
        }
        if (entry->length >= bitmap_size &&
            entry->type == LIMINE_MEMMAP_USABLE) {
            kmem.bitmap = (uint8_t *)PHYS_TO_VIRT(entry->base);
            break;
        }
    }

    memset(kmem.bitmap, 0, bitmap_size);
    klogi("Physical Memory Bitmap Location: %x\n", kmem.bitmap);

    /* Populate bitmap: mark usable pages as free. */
    for (uint64_t i = 0; i < entry_count; i++) {
        struct limine_memmap_entry *entry = res->entries[i];
        if (entry->base + entry->length <= 0x100000) {
            continue;
        }
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            pm_free(entry->base, NUM_PAGES(entry->length));
        }
    }

    /* Mark the bitmap's memory as allocated. */
    pm_allocate(VIRT_TO_PHYS(kmem.bitmap), NUM_PAGES(bitmap_size));
    klogd("Printing usage info...\n");
    pm_used();
    klogs("INIT PM: finished...\n");
}

/**
 * @brief Helper function for printing out physical memory usage
 */
void pm_used() {
    int squared = 1024 * 1024;
    klogi("Memory Total: %d MB\n", kmem.total_size / squared);
    klogd("Physical Base: %x\n", kmem.physical_limit);
    klogd("Virtual Base: %x\n", kmem.physical_limit + MEM_VIRT_OFFSET);
    klogd("Free: %d MB\n", kmem.free_size / squared);
    klogd("Used: %d MB\n",
          (kmem.total_size - kmem.free_size) / squared);
}

/**
 * @brief Optimized: Sets bits in the physical memory bitmap to mark pages
 *        as used
 *
 * @param address Starting address
 * @param num_pages Number of pages to mark
 */
static inline void bitmap_set(uint64_t address, uint64_t num_pages) {
    uint64_t end = address + (num_pages * PAGE_SIZE);
    for (uint64_t i = address; i < end; i += PAGE_SIZE) {
        uint64_t idx = i / (PAGE_SIZE * PAGES_PER_BYTE);
        uint8_t bit = 1 << ((i / PAGE_SIZE) % PAGES_PER_BYTE);
        kmem.bitmap[idx] &= ~bit;
    }
}

/**
 * @brief Checks if pages in the bitmap are free
 *
 * @param address Starting address
 * @param num_pages Number of pages to check
 * @return BITMAP_STATUS Returns ALLOCATED if any page is allocated, FREE
 *         otherwise
 */
static inline BITMAP_STATUS bitmap_free(uint64_t address,
                                        uint64_t num_pages) {
    uint64_t end = address + (num_pages * PAGE_SIZE);
    for (uint64_t i = address; i < end; i += PAGE_SIZE) {
        uint64_t idx = i / (PAGE_SIZE * PAGES_PER_BYTE);
        uint8_t bit = 1 << ((i / PAGE_SIZE) % PAGES_PER_BYTE);
        if (!(kmem.bitmap[idx] & bit)) {
            return ALLOCATED;
        }
    }
    return FREE;
}

/**
 * @brief Marks pages as free in the bitmap
 *
 * @param address Base address to free from
 * @param num_pages Number of physical pages to free
 * @return STATUS SYS_OK if okay, SYS_ERR on failure (e.g. double free).
 */
STATUS pm_free(uint64_t address, uint64_t num_pages) {
    STATUS ret = SYS_OK;
    uint64_t end = address + (num_pages * PAGE_SIZE);
    for (uint64_t i = address; i < end; i += PAGE_SIZE) {
        if (!bitmap_free(i, 1)) {
            kmem.free_size += PAGE_SIZE;
        } else {
            ret = SYS_ERR;
        }
        uint64_t idx = i / (PAGE_SIZE * PAGES_PER_BYTE);
        uint8_t bit = 1 << ((i / PAGE_SIZE) % PAGES_PER_BYTE);
        kmem.bitmap[idx] |= bit;
    }
    return ret;
}

/**
 * @brief Marks pages as allocated in the bitmap
 *
 * @param address Base address to allocate from
 * @param num_pages Number of physical pages to allocate
 * @return STATUS SYS_OK if allocation succeeded, SYS_ERR otherwise
 */
STATUS pm_allocate(uint64_t address, uint64_t num_pages) {
    if (!bitmap_free(address, num_pages)) {
        return SYS_ERR;
    }
    bitmap_set(address, num_pages);
    kmem.free_size -= num_pages * PAGE_SIZE;
    return SYS_OK;
}

/**
 * @brief Retrieves physical pages and returns the starting address
 *
 * @param num_pages Number of pages required
 * @param address Starting search address
 * @param func Function calling pm_get
 * @param line_number Line number in the caller
 * @return uint64_t Starting physical address of allocated pages, or 0 on
 *         failure
 */
uint64_t pm_get(uint64_t num_pages, uint64_t address, const char *func,
                size_t line_number) {
    for (uint64_t i = address; i < kmem.physical_limit; i += PAGE_SIZE) {
        if (pm_allocate(i, num_pages) == SYS_OK) {
            return i;
        }
    }
    kloge("Out of physical memory\n");
    klogd("pm_get: %s:%d attempting to get %d pages from memory (%d "
          "bytes available)\n", func, line_number, num_pages, kmem.free_size);
    halt();
    return 0;
}

/**
 * @brief Maps a page entry into an address space
 *
 * @param address_space Address space to map the entry into
 * @param virt_addr Virtual address for the mapping
 * @param phys_addr Physical address for the mapping
 * @param flags Mapping flags
 */
static void map_page_entry(ADDR_SPACE *address_space, uint64_t virt_addr,
                           uint64_t phys_addr, uint64_t flags) {
    ADDR_SPACE *addr_space = CONVERT_ADDR_SPACE(address_space);
    uint16_t pte = (virt_addr >> 12) & 0x1FF;
    uint16_t pde = (virt_addr >> 21) & 0x1FF;
    uint16_t pdpe = (virt_addr >> 30) & 0x1FF;
    uint16_t pml4e = (virt_addr >> 39) & 0x1FF;

    uint64_t *pml4 = addr_space->pml4;
    uint64_t entry_val = pml4[pml4e];
    uint64_t *pdpt, *pd, *pt;

    /* PDPT Setup */
    if (CHECK_NOT_PRESENT(entry_val)) {
        void *buffer = (void *)pm_get(DEFAULT_PAGES, 0x0, __func__,
                                        __LINE__);
        if (!buffer) {
            klogi("VM: Out of memory for PDPT of PML4 %x\n", pml4);
            halt();
        }
        pdpt = (uint64_t *)PHYS_TO_VIRT(buffer);
        memset(pdpt, 0, PAGE_SIZE * DEFAULT_PAGES);
        pml4[pml4e] = MAKE_TABLE_ENTRY(VIRT_TO_PHYS(pdpt), VM_USERMODE);
        vector_append(&addr_space->memory_list, VIRT_TO_PHYS(pdpt));
    } else {
        pdpt = (uint64_t *)PHYS_TO_VIRT(pml4[pml4e] & ~(0xFFF));
    }

    /* PD Setup */
    entry_val = pdpt[pdpe];
    if (CHECK_NOT_PRESENT(entry_val)) {
        void *buffer = (void *)pm_get(DEFAULT_PAGES, 0x0, __func__,
                                        __LINE__);
        if (!buffer) {
            klogi("VM: Out of memory for PD of PML4 %x\n", pml4);
            halt();
        }
        pd = (uint64_t *)PHYS_TO_VIRT(buffer);
        memset(pd, 0, PAGE_SIZE * DEFAULT_PAGES);
        pdpt[pdpe] = MAKE_TABLE_ENTRY(VIRT_TO_PHYS(pd), VM_USERMODE);
        vector_append(&addr_space->memory_list, VIRT_TO_PHYS(pd));
    } else {
        pd = (uint64_t *)PHYS_TO_VIRT(pdpt[pdpe] & ~(0xFFF));
    }

    /* PT Setup */
    entry_val = pd[pde];
    if (CHECK_NOT_PRESENT(entry_val)) {
        void *buffer = (void *)pm_get(DEFAULT_PAGES, 0x0, __func__,
                                        __LINE__);
        if (!buffer) {
            klogi("VM: Out of memory for PT of PML4 %x\n", pml4);
            halt();
        }
        pt = (uint64_t *)PHYS_TO_VIRT(buffer);
        memset(pt, 0, PAGE_SIZE * DEFAULT_PAGES);
        pd[pde] = MAKE_TABLE_ENTRY(VIRT_TO_PHYS(pt), VM_USERMODE);
        vector_append(&addr_space->memory_list, VIRT_TO_PHYS(pt));
    } else {
        pt = (uint64_t *)PHYS_TO_VIRT(pd[pde] & ~(0xFFF));
    }

    pt[pte] = MAKE_TABLE_ENTRY(phys_addr & ~(0xFFF), flags);

    uint64_t cr3_value = read_cr(cr3);
    if (cr3_value == (uint64_t)(VIRT_TO_PHYS(addr_space->pml4))) {
        __asm__ volatile ("invlpg (%0)" : : "r"(virt_addr));
    }
}

/**
 * @brief Unmaps a page entry from an address space
 *
 * @param addr_space Address space to unmap the page from
 * @param virt_addr Virtual address of the entry
 */
static void unmap_page_entry(ADDR_SPACE *addr_space, uint64_t virt_addr) {
    ADDR_SPACE *as = CONVERT_ADDR_SPACE(addr_space);
    uint16_t pte = (virt_addr >> 12) & 0x1FF;
    uint16_t pde = (virt_addr >> 21) & 0x1FF;
    uint16_t pdpe = (virt_addr >> 30) & 0x1FF;
    uint16_t pml4e = (virt_addr >> 39) & 0x1FF;

    uint64_t *pml4 = as->pml4;
    if (CHECK_NOT_PRESENT(pml4[pml4e])) {
        return;
    }
    uint64_t *pdpt = (uint64_t *)PHYS_TO_VIRT(pml4[pml4e] & ~(0x1FF));
    if (CHECK_NOT_PRESENT(pdpt[pdpe])) {
        return;
    }
    uint64_t *pd = (uint64_t *)PHYS_TO_VIRT(pdpt[pdpe] & ~(0x1FF));
    if (CHECK_NOT_PRESENT(pd[pde])) {
        return;
    }
    uint64_t *pt = (uint64_t *)PHYS_TO_VIRT(pd[pde] & ~(0x1FF));
    if (CHECK_NOT_PRESENT(pt[pte])) {
        return;
    }

    pt[pte] = 0;
    uint64_t cr3_val = read_cr(cr3);
    if (cr3_val == (uint64_t)(VIRT_TO_PHYS(as->pml4))) {
        __asm__ volatile("invlpg (%0)" : : "r"(virt_addr));
    }
    pd[pde] = 0;
    if (pm_free(VIRT_TO_PHYS(pt), DEFAULT_PAGES) == SYS_ERR) {
        kloge("VM: Failed to free pt\n");
        halt();
    }
    for (size_t i = 0; i < vector_len(&as->memory_list); i++) {
        if (vector_at(&as->memory_list, i) == VIRT_TO_PHYS(pt)) {
            vector_erase(&as->memory_list, i);
            break;
        }
    }
    for (size_t i = 0; i < PAGE_SIZE; i++) {
        if (pd[i] != 0) {
            return;
        }
    }
    pdpt[pdpe] = 0;
    if (pm_free(VIRT_TO_PHYS(pd), DEFAULT_PAGES) == SYS_ERR) {
        kloge("VM: Failed to free pd\n");
        halt();
    }
    for (size_t i = 0; i < vector_len(&as->memory_list); i++) {
        if (vector_at(&as->memory_list, i) == VIRT_TO_PHYS(pd)) {
            vector_erase(&as->memory_list, i);
            break;
        }
    }
    for (size_t i = 0; i < PAGE_SIZE; i++) {
        if (pdpt[i] != 0) {
            return;
        }
    }
    pml4[pml4e] = 0;
    if (pm_free(VIRT_TO_PHYS(pdpt), DEFAULT_PAGES) == SYS_ERR) {
        kloge("VM: Failed to free pdpt\n");
        halt();
    }
    for (size_t i = 0; i < vector_len(&as->memory_list); i++) {
        if (vector_at(&as->memory_list, i) == VIRT_TO_PHYS(pdpt)) {
            vector_erase(&as->memory_list, i);
            break;
        }
    }
}

/**
 * @brief Converts a virtual address to a physical address
 *
 * @param addr_space Address space to search in
 * @param virt_addr Virtual address to convert
 * @return uint64_t Physical address, or 0 if not mapped
 */
uint64_t vm_get_phys_addr(ADDR_SPACE *addr_space, uint64_t virt_addr) {
    ADDR_SPACE *as = CONVERT_ADDR_SPACE(addr_space);
    uint16_t pte = (virt_addr >> 12) & 0x1FF;
    uint16_t pde = (virt_addr >> 21) & 0x1FF;
    uint16_t pdpe = (virt_addr >> 30) & 0x1FF;
    uint16_t pml4e = (virt_addr >> 39) & 0x1FF;

    uint64_t *pml4 = as->pml4;
    if (CHECK_NOT_PRESENT(pml4[pml4e])) {
        return 0;
    }
    uint64_t *pdpt = (uint64_t *)PHYS_TO_VIRT(pml4[pml4e] & ~(0x1FF));
    if (CHECK_NOT_PRESENT(pdpt[pdpe])) {
        return 0;
    }
    uint64_t *pd = (uint64_t *)PHYS_TO_VIRT(pdpt[pdpe] & ~(0x1FF));
    if (CHECK_NOT_PRESENT(pd[pde])) {
        return 0;
    }
    uint64_t *pt = (uint64_t *)PHYS_TO_VIRT(pd[pde] & ~(0x1FF));
    if (CHECK_NOT_PRESENT(pt[pte])) {
        return 0;
    }
    return (pt[pte] & 0xFFFFFFFFFFFFF000);
}

/**
 * @brief Unmaps pages from an address space
 *
 * If addr_space is NULL, unmaps from the global memory map
 *
 * @param addr_space Address space to unmap pages from
 * @param virt_addr Virtual address of the first page to unmap
 * @param num_pages Number of pages to unmap
 */
void vm_unmap(ADDR_SPACE *addr_space, uint64_t virt_addr,
              uint64_t num_pages) {
    if (!addr_space) {
        size_t len = vector_len(&global_mem_map);
        for (size_t i = 0; i < len; i++) {
            if (vector_at(&global_mem_map, i).virt_addr == virt_addr) {
                vector_erase(&global_mem_map, i);
                break;
            }
        }
    }
    for (size_t i = 0; i < num_pages * PAGE_SIZE; i += PAGE_SIZE) {
        unmap_page_entry(addr_space, virt_addr + i);
    }
}

/**
 * @brief Maps a number of pages into an address space
 *
 * If addr_space is NULL, the mapping is stored in the global memory map
 *
 * @param addr_space Address space to map pages into
 * @param virt_addr Virtual address for the mapping
 * @param phys_addr Physical address for the mapping
 * @param num_pages Number of pages to map
 * @param flags Mapping flags
 */
void vm_map(ADDR_SPACE *addr_space, uint64_t virt_addr,
            uint64_t phys_addr, uint64_t num_pages,
            uint64_t flags) {
    if (!addr_space) {
        MEM_MAP m = {
            .virt_addr = virt_addr,
            .phys_addr = phys_addr,
            .flags = flags,
            .num_pages = num_pages
        };
        vector_append(&global_mem_map, m);
    }
    for (size_t i = 0; i < num_pages * PAGE_SIZE; i += PAGE_SIZE) {
        map_page_entry(addr_space, virt_addr + i, phys_addr + i, flags);
    }
}

/**
 * @brief Initializes virtual memory for the kernel
 *
 * Sets up the kernel's virtual address space and maps all physical pages
 *
 * @param req Memory map request
 * @param k_req Kernel address request
 */
void vm_init(LIMINE_MEM_REQ req, LIMINE_K_ADDR_REQ k_req) {
    klogs("INIT VM: starting...\n");
    LIMINE_MEM_RES *m = req.response;
    LIMINE_K_ADDR_RES *kernel = k_req.response;
    if (!m) {
        kloge("INIT VM: mem map request is NULL!\n");
        halt();
    } else if (!kernel) {
        kloge("INIT VM: kernel address request is NULL!\n");
        halt();
    }

    kernel_addr_space.pml4 = (void *)(
        PHYS_TO_VIRT(pm_get(DEFAULT_PAGES, 0x0, __func__, __LINE__))
    );
    memset(kernel_addr_space.pml4, 0, PAGE_SIZE * DEFAULT_PAGES);

    size_t num_pages = NUM_PAGES(kmem.physical_limit);
    for (uint64_t i = 0; i < num_pages * PAGE_SIZE; i += PAGE_SIZE) {
        map_page_entry(NULL, MEM_VIRT_OFFSET + i, i, VM_DEFAULT);
    }

    klogi("Mapped %d MB of pages to %x\n",
          kmem.physical_limit / (1024 * 1024), MEM_VIRT_OFFSET);

    for (size_t i = 0; i < m->entry_count; i++) {
        struct limine_memmap_entry *entry = m->entries[i];
        if (entry->type == LIMINE_MEMMAP_KERNEL_AND_MODULES) {
            uint64_t virt_addr = kernel->virtual_base +
              entry->base - kernel->physical_base;
            vm_map(NULL, virt_addr, entry->base,
                   NUM_PAGES(entry->length), VM_DEFAULT);
            klogd("Mapped kernel %x to %x - %x\n", entry->base,
                  virt_addr, virt_addr + entry->length);
            klogd("(length: %d (%d KB), #%d)\n", entry->length,
                  entry->length / 1024, i);
            ENTRY_INFO(entry)
        } else if (entry->type == LIMINE_MEMMAP_FRAMEBUFFER) {
            vm_map(NULL, PHYS_TO_VIRT(entry->base), entry->base,
                   NUM_PAGES(entry->length), VM_DEFAULT);
            klogd("Mapped framebuffer %x to %x - %x\n", entry->base,
                  PHYS_TO_VIRT(entry->base),
                  PHYS_TO_VIRT(entry->base + entry->length));
            klogd("\t(length: %d (%d KB), #%d)\n", entry->length,
                  entry->length / 1024, i);
            ENTRY_INFO(entry)
        } else if (entry->type == LIMINE_MEMMAP_USABLE) {
            int part_bitmap = (VIRT_TO_PHYS(kmem.bitmap) >= entry->base &&
              VIRT_TO_PHYS(kmem.bitmap) < entry->base + entry->length);
            vm_map(NULL, PHYS_TO_VIRT(entry->base), entry->base,
                   NUM_PAGES(entry->length), VM_DEFAULT);
            klogd("Mapped usable %x to %x - %x\n", entry->base,
                  PHYS_TO_VIRT(entry->base),
                  PHYS_TO_VIRT(entry->base + entry->length));
            klogd("\t(length: %d (%d KB), #%d, type: %d, %s)\n",
                  entry->length, entry->length / 1024, i, entry->type,
                  part_bitmap ? "only kernel accessable" :
                  "all tasks accessable");
            ENTRY_INFO(entry)
        } else if (entry->type == LIMINE_MEMMAP_ACPI_RECLAIMABLE) {
            vm_map(NULL, PHYS_TO_VIRT(entry->base), entry->base,
                   NUM_PAGES(entry->length), VM_DEFAULT);
            klogd("Mapped ACPI %x to %x - %x\n", entry->base,
                  PHYS_TO_VIRT(entry->base),
                  PHYS_TO_VIRT(entry->base + entry->length));
            klogd("\t(length: %d (%d KB), #%d)\n", entry->length,
                  entry->length / 1024, i);
            ENTRY_INFO(entry)
        } else if (entry->type ==
                   LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE) {
            vm_map(NULL, PHYS_TO_VIRT(entry->base), entry->base,
                   NUM_PAGES(entry->length), VM_DEFAULT);
            klogd("Mapped BL memory %x to %x - %x\n", entry->base,
                  PHYS_TO_VIRT(entry->base),
                  PHYS_TO_VIRT(entry->base + entry->length));
            klogd("\t(length: %d (%d KB), #%d)\n", entry->length,
                  entry->length / 1024, i);
            ENTRY_INFO(entry)
        } else {
            klogd("NO MAP: ");
            PRINT_MEM_ENTRY_INFO(entry)
        }
    }

    write_cr(cr3, VIRT_TO_PHYS(kernel_addr_space.pml4));
    klogs("INIT VM: finished...\n");
}

/**
 * @brief Creates an address space of the default size
 *
 * Maps all regions from the global memory map into the new address space
 *
 * @return ADDR_SPACE* Pointer to the new address space, or NULL on failure
 */
ADDR_SPACE *create_address_space() {
    ADDR_SPACE *as = kmalloc(sizeof(ADDR_SPACE));
    if (!as) {
        kloge("VM: Cannot create new address space, out of memory!\n");
        return NULL;
    }

    LOCK_LOCK(&vmm_lock);
    memset(as, 0, sizeof(ADDR_SPACE));

    as->pml4 = kmalloc(PAGE_SIZE * DEFAULT_PAGES);
    if (!as->pml4) {
        kloge("VM: Cannot give address space default pages, out of memory!\n");
        kfree(as);
        UNLOCK_LOCK(&vmm_lock);
        return NULL;
    }
    memset(as->pml4, 0, PAGE_SIZE * DEFAULT_PAGES);
    as->lock = LOCK_NEW;

    size_t len = vector_len(&global_mem_map);
    for (size_t i = 0; i < len; i++) {
        MEM_MAP map = vector_at(&global_mem_map, i);
        vm_map(as, map.virt_addr, map.phys_addr, map.num_pages,
               map.flags);
    }
    UNLOCK_LOCK(&vmm_lock);

    klogd("VMM: Created address space at %x (%d pages)\n", as, len);
    return as;
}
