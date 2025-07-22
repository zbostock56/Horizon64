/**
 * @file kmalloc.c
 * @author Zack Bostock
 * @brief Internal kernel memory allocator (slab + chunk hybrid)
 *
 * @copyright Copyright (c) 2025
 */

#include <stddef.h>
#include <stdint.h>

#include <globals.h>
#include <common/kmalloc.h>
#include <common/kprint.h>
#include <common/string.h>
#include <sys/mmu.h>
#include <mm/alloc.h>

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
    void *ptr = NULL;

    if (size > 0 && size < ALLOC_MAX_SIZE) {
        ptr = alloc(size);
        if (ptr)
            return ptr;
        kloge("kmalloc: slab alloc failed for %u bytes at %s:%u, falling back",
              (unsigned)size, func, (unsigned)line);
    }

    // chunk-based allocation
    KMEM_METADATA *mem = (KMEM_METADATA *)
        PHYS_TO_VIRT(pm_get(NUM_PAGES(size) + 1, 0, func, line));

    if (!mem) {
        kloge("kmalloc: out of memory allocating %u bytes at %s:%u",
              (unsigned)size, func, (unsigned)line);
        return NULL;
    }

    /* zero out the memory - unneeded, but nice to have for now */
    memset(mem, 0, size + PAGE_SIZE);

    mem->magic     = KMEM_MAGIC_NUMBER;
    mem->checkno   = kmalloc_checkno++;
    mem->num_pages = NUM_PAGES(size);
    mem->size      = size;
    mem->lineno    = line;
    strncpy(mem->file_name, func, sizeof(mem->file_name) - 1);
    mem->file_name[sizeof(mem->file_name) - 1] = '\0';

    return ((uint8_t *)mem) + PAGE_SIZE;
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
    if (!address)
        return;

    KMEM_METADATA *mem = (KMEM_METADATA *)((uint8_t *)address - PAGE_SIZE);

    if (mem->magic == KMEM_MAGIC_NUMBER && mem->size >= ALLOC_MAX_SIZE) {
        // chunk free path
        pm_free(VIRT_TO_PHYS(mem), mem->num_pages + 1);
        mem->magic = 0;
    } else {
        // slab free path
        free(address);
    }
}

/**
 * @brief Internal kernel implementation of reallocating memory
 *
 * @param address Address of where to reallocate
 * @param new_size New size of the memory allocated
 * @param func Function which is requesting reallocation of memory
 * @param line Line number in the function which is reallocating the memory
 * @return void * Pointer to the reallocated memory
 */
void *krealloc_impl(void *address, size_t new_size, const char *func,
                    size_t line) {
    if (!address) {
        return kmalloc_impl(new_size, func, line);
    }

    if (new_size > 0 && new_size < ALLOC_MAX_SIZE) {
        return realloc(address, new_size);
    }

    // hybrid path: allocate new, copy, free old
    KMEM_METADATA *old = (KMEM_METADATA *)((uint8_t *)address - PAGE_SIZE);
    void *new_ptr = kmalloc_impl(new_size, func, line);
    if (!new_ptr)
        return NULL;

    size_t copy_sz = (old && old->magic == KMEM_MAGIC_NUMBER)
                     ? (old->size < new_size ? old->size : new_size)
                     : new_size;
    memcpy(new_ptr, address, copy_sz);
    kfree_impl(address, func, line);
    return new_ptr;
}

/**
 * @brief Doubles the given buffer
 *
 * @param buffer Pointer to the buffer
 * @param size Pointer to the element which stores the size of the buffer
 * @param unit_size size (in bytes) of each element
 * @return STATUS SYS_OK if success, SYS_ERR if failure
 */
STATUS double_buffer_impl(void **buffer, size_t *size, size_t unit_size,
                          const char *func, size_t line) {
    void *new_buff = krealloc_impl(*buffer, 2 * (*size) * unit_size, func, line);
    if (!new_buff) {
        return SYS_ERR;
    }
    (*buffer) = new_buff;
    *size = 2 * (*size);
    return SYS_OK;
}