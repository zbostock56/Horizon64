/**
 * @file    alloc.c
 * @author  Zack Bostock
 * @brief   Implementation of memory allocation functions (slab caches)
 *
 * @details
 * This file contains the implementation of memory allocation functions for the
 * kernel. It wraps slab allocator caches of various sizes, providing alloc(),
 * free(), realloc(), and initialization.
 *
 * @verbatim
 * Each allocation layout:
 *   ptr:      capacity (bytes) (uint64_t)
 *   ptr + 8:  current size   (uint64_t)
 *   ptr + 16: data region
 *   ptr + 16 + capacity: optional poison (uint64_t)
 * @endverbatim
 *
 * @copyright Copyright (c) 2025
 */

#include <stddef.h>
#include <stdint.h>

#include <globals.h>
#include <common/kprint.h>
#include <common/klib.h>
#include <kconfig.h>
#include <common/memory.h>
#include <common/math.h>
#include <sys/asm.h>
#include <mm/slab.h>
#include <mm/alloc.h>

#define POISON_VALUE    0xDEADBEEFBADC0FFEl
#define CACHE_COUNT     12

#define CAPACITY_SIZE(cache)    ((cache)->size - sizeof(uint64_t) * 2 - \
                                  USE_POISON * sizeof(uint64_t))

static uint64_t allocsizes[CACHE_COUNT] = {
    32, 64, 128, 256, 512, 1024, 2048, 4096,
    8192, 16384, 32768, ALLOC_MAX_SIZE
};

static SCACHE *caches[CACHE_COUNT] = {0};

static void constructor(SCACHE *cache, void *obj) {
    uint64_t *header = obj;
    *header = CAPACITY_SIZE(cache);
    /* zero data region */
    memset(header + 2, 0, *header);
#if USE_POISON
    *((uint64_t *)((uint8_t *)obj + cache->size - sizeof(uint64_t))) = POISON_VALUE;
#endif
}

static void destructor(SCACHE *cache, void *obj) {
#if USE_POISON
    uint64_t poison = *(uint64_t *)((uint8_t *)obj + cache->size - sizeof(uint64_t));
    ASSERT(poison == POISON_VALUE);
#endif
    constructor(cache, obj);
}

static SCACHE *find_cache_from_size(uint64_t size) {
    for (int i = 0; i < CACHE_COUNT; ++i) {
        if (size <= allocsizes[i])
            return caches[i];
    }
    kloge("alloc: cannot get cache for size %u", (unsigned)size);
    halt();
    return NULL; /* unreachable */
}

/** Initialize all slab caches. Must be called before any alloc/free */
void alloc_init() {
    klogs("ALLOC: starting...\n");
    for (int i = 0; i < CACHE_COUNT; i++) {
        uint64_t obj_size = allocsizes[i] +
                            sizeof(uint64_t) *
                            2 + USE_POISON *
                            sizeof(uint64_t);
        caches[i] = slab_newcache(obj_size, 0, constructor, destructor);
        ASSERT(caches[i]);
    }
    klogi("ALLOC: initialized %d slab caches\n", CACHE_COUNT);
    klogs("ALLOC: finished...\n");
}

/**
 * @brief Allocate size bytes from appropriate slab cache
 */
void *alloc(uint64_t size) {
    SCACHE *cache = find_cache_from_size(size);
    uint64_t *allocation = slab_allocate(cache);
    if (!allocation) {
        return NULL;
    }
    ASSERT(*allocation == CAPACITY_SIZE(cache));

    /* store current size */
    *(allocation + 1) = size;
    return (void *)(allocation + 2);
}

/**
 * @brief Free a slab-allocated pointer
 */
void free(void *ptr) {
    if (!ptr)
        return;

    uint64_t *header = (uint64_t *)ptr - 2;
    uint64_t capacity = header[0];
    // uint64_t size = header[1];

    SCACHE *cache = find_cache_from_size(capacity);
    ASSERT(capacity == CAPACITY_SIZE(cache));

#if USE_POISON
    uint64_t poison = *(uint64_t *)((uint8_t *)header + cache->size - sizeof(uint64_t));
    ASSERT(poison == POISON_VALUE);
#endif

    slab_free(cache, header);
}


/**
 * @brief Reallocate a slab-allocated pointer to new size
 */
void *realloc(void *ptr, uint64_t new_size) {
    /* If the pointer is NULL, behave like malloc. */
    if (!ptr)
        return alloc(new_size);

    /* Extract the original allocation metadata:
     * header[0] = capacity (max size of buffer)
     * header[1] = current size (requested size)
     */
    uint64_t *header = (uint64_t *)ptr - 2;
    uint64_t old_capacity = header[0];
    uint64_t old_size     = header[1];

    SCACHE *oldc = find_cache_from_size(old_capacity);
    SCACHE *newc = find_cache_from_size(new_size);

    /* If both allocations would come from the same cache,
     * we can expand the buffer in place (zero out new bytes).
     */
    if (oldc == newc) {
        if (new_size > old_size) {
            memset((uint8_t *)ptr + old_size, 0, new_size - old_size);
        }
        header[1] = new_size;
        return ptr;
    }

    /* Otherwise, allocate a new buffer from a different cache. */
    uint64_t *new_header = slab_allocate(newc);
    if (!new_header)
        return NULL;

    /* Initialize metadata for the new block. */
    new_header[0] = CAPACITY_SIZE(newc);
    new_header[1] = new_size;

    /* Copy old data to new block, up to the minimum of
     * the old size and the new capacity.
     */
    memcpy(new_header + 2, ptr, MIN(old_size, CAPACITY_SIZE(newc)));

    /* Free the old buffer. */
    slab_free(oldc, header);

    /* Return pointer to the data region of the new block. */
    return (void *)(new_header + 2);
}

