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

static KERNEL_MEM_INFO kmem = {0};
vector_new_static(MEM_MAP, global_mem_map);
ADDR_SPACE kernel_addr_space = {0};

/**
 * @brief Physical memory initialization
 * @verbatim
 * Utilize a 8-bit integer as a representation of eight pages, then store in an
 * array. Each page is 4 KB on x86_64.
 *
 * The minimum allocatable is 1 page (which is managed by the requesting process
 * after being allocated using the standard library). The total overhead,
 * assuming that one bit is used per entry is, around 2 MB assuming a total
 * memory size of 64 GB, or around half a 4KB page consumed.
 *
 * @param req Request from Limine bootloader
 */
void pm_init(LIMINE_MEM_REQ req) {
    klogi("INIT PM: starting...\n");
    /* Check validity of request from bootloader */
    LIMINE_MEM_RES *res = req.response;

    if (!res || !res->entry_count) {
        kloge("PM INIT: Memory map request unfulfilled!\n");
        halt();
    }

    kmem.free_size = 0;
    kmem.physical_limit = 0;
    kmem.total_size = 0;

    /* Check entries for their types, their alignment, and their potential overlap */
    for (int i = 0; i < res->entry_count; i++) {
      struct limine_memmap_entry *entry = res->entries[i];

      /* Check what type of memory is being passed from the bootloader and    */
      /* check to see if it is valid memory that can be used. If not, ignore. */
      /* Here, we also add if the memory is marked as kernel or modules since */
      /* we still need this to be available to all processes. */
      if (ENTRY_TYPE_CHECK(entry)) {
        kmem.total_size += entry->length;
      } else {
        continue;
      }

      uint64_t new_limit = entry->base + entry->length;

      if (new_limit > kmem.physical_limit) {
        kmem.physical_limit = new_limit;
        //PRINT_MEM_ENTRY_INFO(entry)
      }
    }

    /* Find a good location for the bitmap */
    uint64_t bitmap_size = kmem.physical_limit / (PAGE_SIZE * PAGES_PER_BYTE);
    for (uint64_t i = 0; i < res->entry_count; i++) {
      struct limine_memmap_entry *entry = res->entries[i];

      /* Try to put the bitmap in higher memory */
      if (entry->base + entry->length <= 0x100000) {
        continue;
      }

      /* LIMINE_MEMMAP_USABLE is guaranteed to be 4K aligned */
      if (entry->length >= bitmap_size && entry->type == LIMINE_MEMMAP_USABLE) {
        kmem.bitmap = (uint8_t *) PHYS_TO_VIRT(entry->base);
        break;
      }
    }

    /* Zero out the memory to set as free */
    memset(kmem.bitmap, 0, bitmap_size);
    klogi("Physical Memory Bitmap Location: %x\n", kmem.bitmap);

    /* Populate the bitmap */
    for (uint64_t i = 0; i < res->entry_count; i++) {
      struct limine_memmap_entry *entry = res->entries[i];

      if (entry->base + entry->length <= 0x100000) {
        continue;
      }

      /* TODO: Add non-aligned sections later to bitmap */
      if (entry->type == LIMINE_MEMMAP_USABLE){
        pm_free(entry->base, NUM_PAGES(entry->length));
      }
    }

    /* Mark bitmap as used memory */
    pm_allocate(VIRT_TO_PHYS(kmem.bitmap), NUM_PAGES(bitmap_size));
    klogi("Printing usage info...\n");
    pm_used();
    klogi("INIT PM: finished...\n");
}

/**
 * @brief Helper function for printing out physical memory which is used.
 */
void pm_used() {
    int squared = 1024 * 1024;
    klogi("Memory total: %d MB | Physcial limit: %x, Virtual Limit %x\n"
          "free: %d MB, used: %d MB\n",
          kmem.total_size / squared, kmem.physical_limit,
          kmem.physical_limit + MEM_VIRT_OFFSET,
          kmem.free_size / squared,
          (kmem.total_size - kmem.free_size) / squared);
}

/**
 * @brief Sets bits int the physical memory bitmap
 *
 * @param address Starting address
 * @param num_pages Number of pages to allocate
 */
static inline void bitmap_set(uint64_t address, uint64_t num_pages) {
  for (uint64_t i = address; i < address + (num_pages * PAGE_SIZE);
       i += PAGE_SIZE) {
    /* Set the page to used by shifting over the number of pages */
    kmem.bitmap[i / (PAGE_SIZE * PAGES_PER_BYTE)] &=
              ~((1 << ((i / PAGE_SIZE) % PAGES_PER_BYTE)));
  }
}

/**
 * @brief Frees pages represented by the bitmap.
 *
 * @param address Starting address
 * @param num_pages Number of pages requested to be freed
 * @return BITMAP_STATUS Returns ALLOCATED if allocated and FREE otherwise
 */
static inline BITMAP_STATUS bitmap_free(uint64_t address, uint64_t num_pages) {
  for (uint64_t i = address; i < address + (num_pages * PAGE_SIZE);
       i += PAGE_SIZE) {
    if (!(kmem.bitmap[i / (PAGE_SIZE * PAGES_PER_BYTE)] &
        (1 << ((i / PAGE_SIZE) % PAGES_PER_BYTE)))) {
      return ALLOCATED;
    }
  }
  return FREE;
}

/**
 * @brief Sets the elements in the bitmap to free starting at some address
 *        for a requested number of pages.
 *
 * @param address Base address to free from
 * @param num_pages number of physical pages to free
 * @return STATUS SYS_OK if okay, SYS_ERR if failure
 */
STATUS pm_free(uint64_t address, uint64_t num_pages) {
  STATUS ret = SYS_OK;
  for (uint64_t i = address; i < address + (num_pages * PAGE_SIZE);
      i += PAGE_SIZE) {
    if (!bitmap_free(i, 1)) {
      kmem.free_size += PAGE_SIZE;
    } else {
      /* Can be used to find double frees if a page is already free */
      ret = SYS_ERR;
    }

    kmem.bitmap[i / (PAGE_SIZE * PAGES_PER_BYTE)] |=
      1 << ((i / PAGE_SIZE) % PAGES_PER_BYTE);
  }

  return ret;
}

/**
 * @brief Sets the elements in the bitmap to allocate starting at some address
 *        for a requested number of pages.
 *
 * @param address Base address to allocate from
 * @param num_pages number of physical pages to allocate
 * @return STATUS SYS_OK if okay, SYS_ERR if failure
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
 * @brief Gets physical pages and returns the number allocated
 *
 * @param num_pages number of pages which were initially allocated
 * @param address base address to where they were allocated
 * @param func function which is calling pm_get
 * @param line_number line number which is calling pm_get
 * @return uint64_t Address of page
 */
uint64_t pm_get(uint64_t num_pages, uint64_t address, const char *func,
                size_t line_number) {
    for (uint64_t i = address; i < kmem.physical_limit; i += PAGE_SIZE) {
        if (pm_allocate(i, num_pages) == SYS_OK) {
            return i;
        }
    }

   kloge("Out of physical memory\n");
   klogi("pm_get: %s:%d attempting to get %d pages from memory"
         "(%d bytes available)\n", func, line_number, num_pages, kmem.free_size);
   halt();

   /* Should never make it past this point */

   /* We use this to represent NULL here */
   return 0;
}

/**
 * @brief Maps a page entry to an address space
 *
 * @param address_space Address apce to map entry to
 * @param virt_addr Virtual address of page entry
 * @param phys_addr Physical address of page entry
 * @param flags Virtual memory flags entry
 */
static void map_page_entry(ADDR_SPACE *address_space, uint64_t virt_addr,
                           uint64_t phys_addr, uint64_t flags) {
    ADDR_SPACE *addr_space = CONVERT_ADDR_SPACE(address_space);

    uint16_t pte = (virt_addr >> 12) & 0x1FF;
    uint16_t pde = (virt_addr >> 21) & 0x1FF;
    uint16_t pdpe = (virt_addr >> 30) & 0x1FF;
    uint16_t pml4e = (virt_addr >> 39) & 0x1FF;

    uint64_t *pml4 = addr_space->pml4;
    uint64_t *pdpt;
    uint64_t *pd;
    uint64_t *pt;

    /* Set up levels of page */
    pdpt = (uint64_t *) PHYS_TO_VIRT(pml4[pml4e] & ~(0xFFF));
    if (CHECK_NOT_PRESENT(pml4[pml4e])) {
        void *buffer = (void *) pm_get(DEFAULT_PAGES, 0x0, __func__, __LINE__);
        if (!buffer) {
            klogi("VM: Out of memory for PDPT of PML4 %x\n", pml4);
            halt();
        }
        pdpt = (uint64_t *) PHYS_TO_VIRT(buffer);
        memset(pdpt, 0, PAGE_SIZE * DEFAULT_PAGES);
        pml4[pml4e] = MAKE_TABLE_ENTRY(VIRT_TO_PHYS(pdpt), VM_USERMODE);
        vector_append(&addr_space->memory_list, VIRT_TO_PHYS(pdpt));
    }

    pd = (uint64_t *) PHYS_TO_VIRT(pdpt[pdpe] & ~(0xFFF));
    if (CHECK_NOT_PRESENT(pdpt[pdpe])) {
        void *buffer = (void *) pm_get(DEFAULT_PAGES, 0x0, __func__, __LINE__);
        if (!buffer) {
            klogi("VM: Out of memory for PD of PML4 %x\n", pml4);
            halt();
        }
        pd = (uint64_t *) PHYS_TO_VIRT(buffer);
        memset(pd, 0, PAGE_SIZE * DEFAULT_PAGES);
        pdpt[pdpe] = MAKE_TABLE_ENTRY(VIRT_TO_PHYS(pd), VM_USERMODE);
        vector_append(&addr_space->memory_list, VIRT_TO_PHYS(pd));
    }

    pt = (uint64_t *) PHYS_TO_VIRT(pd[pde] & ~(0xFFF));
    if (CHECK_NOT_PRESENT(pd[pde])) {
        void *buffer = (void *) pm_get(DEFAULT_PAGES, 0x0, __func__, __LINE__);
        if (!buffer) {
            klogi("VM: Out of memory for PT of PML4 %x\n", pml4);
            halt();
        }
        pt = (uint64_t *) PHYS_TO_VIRT(buffer);
        memset(pt, 0, PAGE_SIZE * DEFAULT_PAGES);
        pd[pde] = MAKE_TABLE_ENTRY(VIRT_TO_PHYS(pt), VM_USERMODE);
        vector_append(&addr_space->memory_list, VIRT_TO_PHYS(pt));
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
 * @param addr_space Address space to unmap page from
 * @param virt_addr Virtual address of page entry
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

    uint64_t *pdpt = (uint64_t *) PHYS_TO_VIRT(pml4[pml4e] & ~(0x1FF));
    if (CHECK_NOT_PRESENT(pdpt[pdpe])) {
        return;
    }

    uint64_t *pd = (uint64_t *) PHYS_TO_VIRT(pdpt[pdpe] & ~(0x1FF));
    if (CHECK_NOT_PRESENT(pd[pde])) {
        return;
    }

    uint64_t *pt = (uint64_t *) PHYS_TO_VIRT(pd[pde] & ~(0x1FF));
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
        klogi("VM: Failed to free pt\n");
        halt();
    }

    size_t i;
    for (i = 0; i < vector_len(&as->memory_list); i++) {
        if (vector_at(&as->memory_list, i) == VIRT_TO_PHYS(pt)) {
            vector_erase(&as->memory_list, i);
            break;
        }
    }

    for (i = 0; i < PAGE_SIZE; i++) {
        if (pd[i] != 0) {
            return;
        }
    }

    pdpt[pdpe] = 0;
    if (pm_free(VIRT_TO_PHYS(pd), DEFAULT_PAGES) == SYS_ERR) {
        klogi("VM: Failed to free pd\n");
        halt();
    }

    for (i = 0; i < vector_len(&as->memory_list); i++) {
        if (vector_at(&as->memory_list, i) == VIRT_TO_PHYS(pd)) {
            vector_erase(&as->memory_list, i);
            break;
        }
    }

    for (i = 0; i < PAGE_SIZE; i++) {
        if (pdpt[i] != 0) {
            return;
        }
    }

    pml4[pml4e] = 0;
    if (pm_free(VIRT_TO_PHYS(pdpt), DEFAULT_PAGES) == SYS_ERR) {
        klogi("VM: Failed to free pdpt\n");
        halt();
    }

    for (i = 0; i < vector_len(&as->memory_list); i++) {
        if (vector_at(&as->memory_list, i) == VIRT_TO_PHYS(pdpt)) {
            vector_erase(&as->memory_list, i);
            break;
        }
    }
}

/**
 * @brief Converts the virtual address into a physical address
 *
 * @param addr_space Address space to get entry from
 * @param virt_addr Virtual address of entry
 * @return uint64_t Physical address
 */
uint64_t vm_get_phys_addr(ADDR_SPACE *addr_space, uint64_t virt_addr) {
    ADDR_SPACE *as = CONVERT_ADDR_SPACE(addr_space);

    uint16_t pte = (virt_addr >> 12) & 0x1FF;
    uint16_t pde = (virt_addr >> 21) & 0x1FF;
    uint16_t pdpe = (virt_addr >> 30) & 0x1FF;
    uint16_t pml4e = (virt_addr >> 39) & 0x1FF;

    uint64_t *pml4 = as->pml4;
    if (CHECK_NOT_PRESENT(pml4[pml4e])) {
        return (uint64_t) NULL;
    }

    uint64_t *pdpt = (uint64_t *) PHYS_TO_VIRT(pml4[pml4e] & ~(0x1FF));
    if (CHECK_NOT_PRESENT(pdpt[pdpe])) {
        return (uint64_t) NULL;
    }

    uint64_t *pd = (uint64_t *) PHYS_TO_VIRT(pdpt[pdpe] & ~(0x1FF));
    if (CHECK_NOT_PRESENT(pd[pte])) {
        return (uint64_t) NULL;
    }

    uint64_t *pt = (uint64_t *) PHYS_TO_VIRT(pd[pde] & ~(0x1FF));
    if (CHECK_NOT_PRESENT(pt[pte])) {
        return (uint64_t) NULL;
    }

    return (pt[pte] & 0xFFFFFFFFFFFFF000);
}

/**
 * @brief Unmaps a page from an address space
 *
 * @param addr_space Address space to unmap page from
 * @param virt_addr Virtual address of the first page to unmap
 * @param num_pages Number of pages to unmap
 */
void vm_unmap(ADDR_SPACE *addr_space, uint64_t virt_addr, uint64_t num_pages) {
    if (!addr_space) {
        /* Have to unmap the corresponding virtual address */
        size_t len = vector_len(&global_mem_map);
        for (size_t i = 0; i < len; i++) {
            if (vector_at(&global_mem_map, i).virt_addr == virt_addr) {
                vector_erase(&global_mem_map, i)
                break;
            }
        }
    }

    for (size_t i = 0; i < num_pages * PAGE_SIZE; i += PAGE_SIZE) {
        unmap_page_entry(addr_space, virt_addr + i);
    }
}

/**
 * @brief Maps a number of pages to an address space
 *
 * @param addr_space Address space to map pages to
 * @param virt_addr Virtual address of the pages
 * @param phys_addr Physical address of the pages
 * @param num_pages Number of pages to map
 * @param flags Flags of the pages to map
 */
void vm_map(ADDR_SPACE *addr_space, uint64_t virt_addr, uint64_t phys_addr,
            uint64_t num_pages, uint64_t flags) {
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
 * @brief Main initialization function for virtual memory. Sets up kernel's
 *        virtual memory address space.
 *
 * @param req Memory map request
 * @param k_req Kernel address request
 */
void vm_init(LIMINE_MEM_REQ req, LIMINE_K_ADDR_REQ k_req) {
    klogi("INIT VM: starting...\n");
    /* Check inputs from the bootloader */
    LIMINE_MEM_RES *m = req.response;
    LIMINE_K_ADDR_RES *kernel = k_req.response;
    if (!m) {
        kloge("INIT VM: mem map request is NULL!\n");
        halt();
    } else if (!kernel) {
        kloge("INIT VM: kernel address request is NULL!\n");
        halt();
    }

    size_t i;

    kernel_addr_space.pml4 = kmalloc(DEFAULT_PAGES * PAGE_SIZE);
    memset(kernel_addr_space.pml4, 0, PAGE_SIZE * DEFAULT_PAGES);

    uint64_t mem_size = 1024 * 512;

    uint64_t min = NUM_PAGES(kmem.physical_limit) < mem_size ?
                   NUM_PAGES(kmem.physical_limit) : mem_size;

    vm_map(NULL, MEM_VIRT_OFFSET, 0, min, VM_USERMODE);

    /* Map the number of pages up to the physical limit */
    size_t num_pages = NUM_PAGES(kmem.physical_limit);
    for (i = 0; i < num_pages * PAGE_SIZE; i += PAGE_SIZE) {
        map_page_entry(NULL, MEM_VIRT_OFFSET + i, i, VM_DEFAULT);
    }

    klogi("Mapped %d MB of pages to %x\n", kmem.physical_limit / (1024 * 1024),
                                           MEM_VIRT_OFFSET);

    for (i = 0; i < m->entry_count; i++) {
        struct limine_memmap_entry *entry = m->entries[i];

        if (entry->type == LIMINE_MEMMAP_KERNEL_AND_MODULES) {
            uint64_t virt_addr = kernel->virtual_base +
                                 entry->base - kernel->physical_base;
            /* Should share for all processes */
            vm_map(NULL, virt_addr, entry->base, NUM_PAGES(entry->length),
                   VM_DEFAULT);
            klogi("Mapped kernel %x to %x - %x\n"
                  "\t(length: %d (%d KB), #%d)\n\t",
                  entry->base, virt_addr, virt_addr + entry->length,
                  entry->length, entry->length / 1024, i);
            ENTRY_INFO(entry)
        } else if (entry->type == LIMINE_MEMMAP_FRAMEBUFFER) {
            vm_map(NULL, PHYS_TO_VIRT(entry->base), entry->base,
                   NUM_PAGES(entry->length), VM_DEFAULT);
            klogi("Mapped framebuffer %x to %x - %x\n"
                  "\t(length: %d (%d KB), #%d)\n\t",
                  entry->base, PHYS_TO_VIRT(entry->base),
                  PHYS_TO_VIRT(entry->base + entry->length),
                  entry->length, entry->length / 1024, i);
            ENTRY_INFO(entry)
        } else if (entry->type == LIMINE_MEMMAP_USABLE) {
            int part_bitmap = 0;
            if (VIRT_TO_PHYS(kmem.bitmap) >= entry->base &&
                VIRT_TO_PHYS(kmem.bitmap) < entry->base + entry->length) {
                part_bitmap = 1;
            }
            vm_map(NULL, PHYS_TO_VIRT(entry->base), entry->base,
                   NUM_PAGES(entry->length), VM_DEFAULT);
            klogi("Mapped usable %x to %x - %x\n"
                  "\t(length: %d (%d KB), #%d, type: %d, %s)\n\t",
                  entry->base, PHYS_TO_VIRT(entry->base),
                  PHYS_TO_VIRT(entry->base + entry->length),
                  entry->length, entry->length / 1024, i, entry->type,
                  part_bitmap ? "only kernel accessable" :
                  "all tasks accessable");
            ENTRY_INFO(entry)
        } else if (entry->type == LIMINE_MEMMAP_ACPI_RECLAIMABLE) {
            vm_map(NULL, PHYS_TO_VIRT(entry->base), entry->base,
                  NUM_PAGES(entry->length), VM_DEFAULT);
            klogi("Mapped ACPI %x to %x - %x\n"
                  "\t(length: %d (%d KB), #%d)\n\t",
                  entry->base, PHYS_TO_VIRT(entry->base),
                  PHYS_TO_VIRT(entry->base + entry->length),
                  entry->length, entry->length / 1024, i);
            ENTRY_INFO(entry)
        } else if (entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE) {
            vm_map(NULL, PHYS_TO_VIRT(entry->base), entry->base,
                  NUM_PAGES(entry->length), VM_DEFAULT);
            klogi("Mapped BL memory %x to %x - %x\n"
                  "\t(length: %d (%d KB), #%d)\n\t",
                  entry->base, PHYS_TO_VIRT(entry->base),
                  PHYS_TO_VIRT(entry->base + entry->length),
                  entry->length, entry->length / 1024, i);
            ENTRY_INFO(entry)
        } else {
            /* Skip over these entries since we don't want to use them as     */
            /* accessable memory. Just print out their info to give a better  */
            /* picture of what the memory space looks like.                   */
            klogi("NO MAP: ");
            PRINT_MEM_ENTRY_INFO(entry)
        }
    }

    write_cr(cr3, VIRT_TO_PHYS(kernel_addr_space.pml4));
    klogi("INIT VM: finished...\n");
}

/**
 * @brief creates an address space of the default size
 */
ADDR_SPACE *create_address_space() {
    ADDR_SPACE *as = kmalloc(sizeof(ADDR_SPACE));

    if (!as) {
        kloge("VM: Cannot create new address space, out of memory!\n");
        return NULL;
    }

    memset(as, 0, sizeof(ADDR_SPACE));

    as->pml4 = kmalloc(PAGE_SIZE * DEFAULT_PAGES);
    if (!as->pml4) {
        kloge("VM: Cannot give address space default pages, out of memory!\n");
        kfree(as);
        return NULL;
    }
    as->lock = LOCK_NEW();

    for (size_t i = 0; i < vector_len(&global_mem_map); i++) {
        MEM_MAP map = vector_at(&global_mem_map, i);
        vm_map(as, map.virt_addr, map.phys_addr, map.num_pages, map.flags);
    }
    return as;
}
