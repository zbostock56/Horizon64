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
#include <common/math.h>
#include <common/klib.h>
#include <sys/mmu.h>
#include <mm/alloc.h>

#if KMEM_DEBUG
size_t kmalloc_checkno = 0;
#endif

/**
 * @brief Internal kernel implementation of malloc
 *
 * @param size Number of bytes to allocate
 * @param func Function name who allocated the bytes
 * @param line Line number which the allocation was called
 * @return void * Pointer to the data which was allocated
 */
void *kmalloc_core(uint64_t size, const char *func, size_t line) {
#if SLAB_ALLOCATOR
    if (size > 0 && size < ALLOC_MAX_SIZE) {
        void *ptr = NULL;
        ptr = alloc(size);
        if (!ptr) {
            kloge("kmalloc: slab alloc failed for %d bytes at %s:%d, "
                  "falling back to chunk allocator\n",
                    size, func, line);
        } else {
            return ptr;
        }
    }
#endif
    return kmalloc_chunk(size, func, line);
}

/**
 * @brief Specific function for allocating using bitmap allocator
 *
 * @param size Number of bytes to allocate
 * @param func Function name who allocated the bytes
 * @param line Line number which the allocation was called
 * @return void * Pointer to the data which was allocated
 */
void *kmalloc_chunk(uint64_t size, const char *func, size_t line) {
    KMEM_METADATA *mem = (KMEM_METADATA *)
        PHYS_TO_VIRT(pm_get(NUM_PAGES(size) + 1, 0, func, line));

    if (!mem) {
        kloge("kmalloc: out of memory allocating %u bytes at %s:%u",
              (unsigned)size, func, (unsigned)line);
        return NULL;
    }

#if KMEM_DEBUG
    /* zero out the memory - unneeded, but nice to have */
    memset(mem, 0, size + PAGE_SIZE);
#endif

    /* Setup metadata */
    mem->magic     = KMEM_MAGIC_NUMBER;
    mem->num_pages = NUM_PAGES(size);
    mem->size      = size;
    mem->lineno    = line;
#if KMEM_DEBUG
    mem->checkno   = kmalloc_checkno;
#endif
    strncpy(mem->file_name, func, sizeof(mem->file_name) - 1);
    mem->file_name[sizeof(mem->file_name) - 1] = '\0';

    return ((uint8_t *)mem) + PAGE_SIZE;
}

/**
 * @brief Helper for checking null pointers for freeing
 *
 * @param address Address to check NULL on
 * @param internal Internal function which called this function
 * @param func Function which is calling free
 * @param line Line number which is calling free
 *
 * @returns STATUS SYS_OK if not NULL, SYS_ERR if NULL
 */
static inline STATUS null_check(void *address, const char *internal,
                                const char *func, size_t line) {
    if (!address) {
#if KMEM_DEBUG
        kloge("%s: Trying to free a NULL pointer from %s:%d\n", internal, func,
                line);
#else
        (void) func;
        (void) internal;
        (void) line;
#endif
        return SYS_ERR;
    }
    return SYS_OK;
}

/**
 * @brief Internal kernel implementation free memory
 *
 * @param address Address of where to free
 * @param func Function name which is freeing the memory
 * @param line Line number in the function which is freeing the memory
 */
void kfree_core(void *address, const char *func, size_t line) {
#if KMEM_DEBUG
        klogd("Freeing memory from (%s:%d)\n", func, line);
#endif

#if SLAB_ALLOCATOR
    if (null_check(address, __func__, func, line) == SYS_ERR) {
        return;
    }
    uint64_t *ptr = (uint64_t *)address;
    uint64_t size = *(ptr - 1);
    if (size > 0 && size < ALLOC_MAX_SIZE) {
        free(address);
        return;
    }
#endif
    kfree_chunk(address, func, line);
    return;
}

/**
 * @brief Internal kernel implementation free memory for bitmap allocator
 *
 * @param address Address of where to free
 * @param func Function name which is freeing the memory
 * @param line Line number in the function which is freeing the memory
 */
void kfree_chunk(void *address, const char *func, size_t line) {
    if (null_check(address, __func__, func, line) == SYS_ERR) {
        return;
    }
    KMEM_METADATA *mem = (KMEM_METADATA *)((uint8_t *)address - PAGE_SIZE);

    if (mem->magic == KMEM_MAGIC_NUMBER) {
#if KMEM_DEBUG
        klogd("kfree_chunk: freeing memory of size %d from (%s:%d)\n",
                mem->size, func, line);
#endif
        pm_free(VIRT_TO_PHYS(address), mem->num_pages + 1);
        mem->magic = 0;
    } else {
#if KMEM_DEBUG
        mem_debug();
#endif
        kloge("kfree_chunk: memory corruption detected\n");
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
/**
 * @brief Internal kernel implementation of reallocating memory
 *
 * @param address Address of where to reallocate
 * @param new_size New size of the memory allocation
 * @param func Function which is requesting reallocation of memory
 * @param line Line number in the function which is reallocating the memory
 * @return void * Pointer to the reallocated memory
 */
void *krealloc_core(void *address, size_t new_size, const char *func,
                    size_t line) {
#if KMEM_DEBUG
    if (new_size >= ALLOC_MAX_SIZE) {
        klogd("krealloc: reallocating %d bytes (>= %d)\n",
              new_size, (size_t) ALLOC_MAX_SIZE);
    }
#endif

#if SLAB_ALLOCATOR
    if (!address) {
        return kmalloc_core(new_size, func, line);
    }

    uint64_t *buf = (uint64_t *) address;
    void *new_address = kmalloc_core(new_size, func, line);

    if (new_address) {
        memcpy(new_address, address, MIN(*(buf - 1), new_size));
    }

    kfree_core(address, func, line);
    return new_address;
#else
    return krealloc_chunk(address, new_size, func, line);
#endif
}


/**
 * @brief Internal kernel implementation of reallocating memory for bitmap
 *        allocator
 *
 * @param address Address of where to reallocate
 * @param new_size New size of the memory allocated
 * @param func Function which is requesting reallocation of memory
 * @param line Line number in the function which is reallocating the memory
 * @return void * Pointer to the reallocated memory
 */
void *krealloc_chunk(void *address, size_t new_size, const char *func,
                    size_t line) {
    if (!address) {
        return kmalloc_chunk(new_size, func, line);
    }

    KMEM_METADATA *mem = (KMEM_METADATA *) ((uint8_t *) address - PAGE_SIZE);

    if (NUM_PAGES(mem->size) == NUM_PAGES(new_size)) {
        /* Number of pages is the same, just update metadata */
        mem->size = new_size;
        mem->num_pages = NUM_PAGES(new_size);
        mem->magic = KMEM_MAGIC_NUMBER;
        mem->lineno = line;
        strncpy(mem->file_name, func, sizeof(mem->file_name));
        mem->file_name[sizeof(mem->file_name) - 1] = '\0';
        return address;
    }

    /* Number of pages differs, allocate new region */
    void *new_base = kmalloc_chunk(new_size, func, line);

#if KMEM_DEBUG
    /* zero new memory */
    memset(new_base, 0, new_size);
#endif

    if (mem->size > new_size) {
        memcpy(new_base, address, new_size);
    } else {
        memcpy(new_base, address, mem->size);
    }

    kfree_chunk(address, func, line);
    return new_base;
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
    void *new_buff = krealloc_core(*buffer, 2 * (*size) * unit_size, func, line);
    if (!new_buff) {
        return SYS_ERR;
    }
    (*buffer) = new_buff;
    *size = 2 * (*size);
    return SYS_OK;
}