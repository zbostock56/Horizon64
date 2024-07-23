/**
 * @file kmalloc.c
 * @author Zack Bostock
 * @brief Internal kernel memory allocator
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <stddef.h>

#include <common/kmalloc.h>
#include <common/kprint.h>
#include <common/string.h>
#include <sys/asm.h>
#include <sys/mmu.h>

size_t kmalloc_checkno = 0;

/**
 * @brief Internal kernel implementation of malloc
 *
 * @param size Number of bytes to allocate
 * @param func Function name who allocated the bytes
 * @param line Line number which the allocation was called
 * @return void * Pointer to the data which was allocated
 */
void *kmalloc_impl(uint64_t size, const char *func, size_t line) {
    KMEM_METADATA *mem = (KMEM_METADATA *)
        PHYS_TO_VIRT(pm_get(NUM_PAGES(size) + 1, 0x0, func, line));

    if (!mem) {
        kloge("Out of memory when allocating %d bytes from %s:%d\n", size,
                func, line);
    }

    /* zero out the memory - unneeded, but nice to have for now */
    memset(mem, 0, size + PAGE_SIZE);

    mem->magic = KMEM_MAGIC_NUMBER;
    mem->checkno = kmalloc_checkno;
    mem->num_pages = NUM_PAGES(size);
    mem->size = size;
    mem->lineno = line;
    strncpy(mem->file_name, func, sizeof(mem->file_name) - 1);

    return ((uint8_t *) mem) + PAGE_SIZE;
}

/**
 * @brief Internal kernel implementation free memory
 *
 * @param address Address of where to free
 * @param func Function name which is freeing the memory
 * @param line Line number in the function which is freeing the memory
 */
void kfree_impl(void *address, const char *func, size_t line) {
    (void) func;
    (void) line;

    KMEM_METADATA *mem = (KMEM_METADATA *) ((uint8_t *) address - PAGE_SIZE);

    if (mem->magic == KMEM_MAGIC_NUMBER) {
        pm_free(VIRT_TO_PHYS(mem), mem->num_pages + 1);
        mem->magic = 0;
    } else {
        kloge("free: memory corruption detected\n");
    }
}

/**
 * @brief Internal kernel implementation of reallocating memory
 *
 * @param address Address of where to reallocate
 * @param new_size New side of the memory allocated
 * @param func Function which is requesting reallocation of memory
 * @param line Line number in the function which is reallocating the memory
 * @return void * Pointer to the reallocated memory
 */
void *krealloc_impl(void *address, size_t new_size, const char *func,
                    size_t line) {
    if (!address) {
        return kmalloc_impl(new_size, func, line);
    }

    KMEM_METADATA *mem = (KMEM_METADATA *) ((uint8_t *) address - PAGE_SIZE);

    if (NUM_PAGES(mem->size) == NUM_PAGES(new_size)) {
        /* Number of pages is the same, don't change number of pages alloc'd */
        mem->size = new_size;
        mem->num_pages = NUM_PAGES(new_size);
        mem->magic = KMEM_MAGIC_NUMBER;
        mem->lineno = line;
        strncpy(mem->file_name, func, sizeof(mem->file_name) - 1);
        return address;
    }

    /* Number of pages is different, allocate more pages */
    void *new_base = kmalloc_impl(new_size, func, line);

    /* nice to have, but unneeded */
    memset(new_base, 0, new_size);

    if (mem->size > new_size) {
        memcpy(new_base, address, new_size);
    } else {
        memcpy(new_base, address, mem->size);
    }

    kfree_impl(address, func, line);
    return new_base;
}
