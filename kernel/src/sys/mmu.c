#include <sys/mmu.h>

/*
  Physcial Memory Initialization

  Utilize a 8-bit integer as a representation of eight pages, then store in an
  array. Each page is 4 KB on x86_64.

  The minimum allocatable is 1 page (which is managed by the requesting process
  after being allocated using the standard library). The total overhead,
  assuming that one bit is used per entry is, around 2 MB assuming a total
  memory size of 64 GB, or around half a 4KB page consumed.
*/
void pm_init(LIMINE_MEM_REQ req) {
    /* Check validity of request from bootloader */
    if (!req.response || !req.response->entry_count) {
        kloge("ERROR: Memory map request unfulfilled!\n");
        halt();
    }

    kmem.free_size = 0;
    kmem.physical_limit = 0;
    kmem.total_size = 0;

    /* Check entries for their types, their alignment, and their potential overlap */
    for (int i = 0; i < req.response->entry_count; i++) {
      struct limine_memmap_entry *entry = req.response->entries[i];
      
      /* Check what type of memory is being passed from the bootloader and    */
      /* check to see if it is valid memory that can be used. If not, ignore. */
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
    for (uint64_t i = 0; i < req.response->entry_count; i++) {
      struct limine_memmap_entry *entry = req.response->entries[i];
      
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

    klogi("Total memory size is:\n");
    PRINT_MEM_SIZE(kmem.total_size);

    /* Populate the bitmap */
    for (uint64_t i = 0; i < req.response->entry_count; i++) {
      struct limine_memmap_entry *entry = req.response->entries[i];

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
    klogi("Physical Memory initialization finished...\n");
    klogi("Memory total: %d MB | Physcial limit: %x, free: %d MB, used: %d MB\n",
          kmem.total_size / (1024 * 1024), kmem.physical_limit,
          kmem.free_size / (1024 * 1024),
          (kmem.total_size - kmem.free_size) / (1024 * 1024));
}

/* 
  Given a starting address, set the pages to used for the number of
  pages requested.
*/
static inline void bitmap_set(uint64_t address, uint64_t num_pages) {
  for (uint64_t i = address; i < address + (num_pages * PAGE_SIZE);
       i += PAGE_SIZE) {
    /* Set the page to used by shifting over the number of pages */
    kmem.bitmap[i / (PAGE_SIZE * PAGES_PER_BYTE)] &=
              ~((1 << ((i / PAGE_SIZE) % PAGES_PER_BYTE)));
  }
}

/* 
  Given a starting address, check if the number of pages requested
  are free or allocated.
*/
static inline int bitmap_free(uint64_t address, uint64_t num_pages) {
  for (uint64_t i = address; i < address + (num_pages * PAGE_SIZE);
       i += PAGE_SIZE) {
    if (!(kmem.bitmap[i / (PAGE_SIZE * PAGES_PER_BYTE)] &
        (1 << ((i / PAGE_SIZE) % PAGES_PER_BYTE)))) {
      return ALLOCATED;
    }
  }
  return FREE;
}

/*
  Sets the elements in the bitmap to free starting at some address
  for a requested number of pages.
*/
STATUS pm_free(uint64_t address, uint64_t num_pages) {
  STATUS ret = SYS_OK;
  for (uint64_t i = address; i < address + (num_pages * PAGE_SIZE);
      i += PAGE_SIZE) {
    if (!bitmap_free(i, 1)) {
      kmem.free_size += PAGE_SIZE;
    } else {
      /* Can be used to find double frees if a page is already free */
      ret = SYS_ERROR;
    }

    kmem.bitmap[i / (PAGE_SIZE * PAGES_PER_BYTE)] |=
      1 << ((i / PAGE_SIZE) % PAGES_PER_BYTE);
  }

  return ret;
}

/*
  Sets the elements in the bitmap to free starting at some address
  for a requested number of pages.
*/
STATUS pm_allocate(uint64_t address, uint64_t num_pages) {
  if (!bitmap_free(address, num_pages)) {
    return SYS_OK;
  }

  bitmap_set(address, num_pages);
  kmem.free_size -= num_pages * PAGE_SIZE;
  return SYS_ERROR;
}