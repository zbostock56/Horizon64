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
#include <kconfig.h>
#include <common/kprint.h>
#include <common/klib.h>
#include <string.h>
#include <common/math.h>
#include <sys/asm.h>
#include <mm/slab.h>
#include <mm/alloc.h>



/**
 * @brief Total number of predefined slab caches
 */
#define CACHE_COUNT     12

/**
 * @brief Computes the usable capacity of a cache object
 *
 * @param cache Pointer to the slab cache
 * @return Usable capacity in bytes, accounting for header and optional poison value
 */
#define CAPACITY_SIZE(cache) ((cache)->size - sizeof(uint64_t) * 2 - \
                              SLAB_POISON * sizeof(uint64_t))


/**
 * @brief Predefined allocation sizes (in bytes) for slab caches
 *
 * These correspond to increasing power-of-two sizes up to ALLOC_MAX_SIZE.
 */
static uint64_t allocsizes[CACHE_COUNT] = {
    32, 64, 128, 256, 512, 1024, 2048, 4096,
    8192, 16384, 32768, ALLOC_MAX_SIZE
};

/**
 * @brief Array of slab cache pointers corresponding to each allocation size
 *
 * Initialized by alloc_init().
 */
static SCACHE *caches[CACHE_COUNT] = {0};

/**
 * @brief Initializes an object within a slab cache
 *
 * Sets the object's header with its capacity and zeroes the data region.
 * If SLAB_POISON is enabled, a poison value is written at the end of the object.
 *
 * @param cache The slab cache the object belongs to
 * @param obj Pointer to the object to initialize
 */
static void constructor(SCACHE *cache, void *obj) {
    uint64_t *header = obj;
    *header = CAPACITY_SIZE(cache);

    /* zero data region */
    memset(header + 2, 0, *header);

#if SLAB_POISON
    *((uint64_t *)((uint8_t *)obj + cache->size - sizeof(uint64_t))) = POISON_VALUE;
#endif
}

/**
 * @brief Prepares an object for reuse by re-running the constructor
 *
 * Also checks for the poison value if enabled, to detect corruption.
 *
 * @param cache The slab cache the object belongs to
 * @param obj Pointer to the object to destruct
 */
static void destructor(SCACHE *cache, void *obj) {
#if SLAB_POISON
    uint64_t poison = *(uint64_t *)((uint8_t *)obj + cache->size - sizeof(uint64_t));
    ASSERT(poison == POISON_VALUE);
#endif

    /* Reinitialize for use again */
    constructor(cache, obj);
}

/**
 * @brief Helper to finding a cache block from a specific size
 *
 * @param size Size to find
 * @return SCACHE * Found block or halts CPU if not found
 */
static SCACHE *find_cache_from_size(uint64_t size) {
    for (int i = 0; i < CACHE_COUNT; ++i) {
        if (size <= allocsizes[i])
            return caches[i];
    }
    kloge("alloc: cannot get cache for size %u", (unsigned)size);
    halt();
    return NULL;
}

/**
 * @brief Slab allocator initialization function
 *
 * @note Must be called before alloc/free
 */
void alloc_init() {
    klogs("ALLOC: starting...\n");
    for (int i = 0; i < CACHE_COUNT; i++) {
        uint64_t obj_size = allocsizes[i] +
                            sizeof(uint64_t) *
                            2 + SLAB_POISON *
                            sizeof(uint64_t);
        caches[i] = slab_newcache(obj_size, 0, constructor, destructor);
        ASSERT(caches[i]);
    }
    klogi("ALLOC: initialized %d slab caches\n", CACHE_COUNT);
    klogs("ALLOC: finished...\n");
}

/**
 * @brief Allocate a memory buffer of the given size using the slab allocator
 *
 * Chooses the smallest slab cache that can accommodate the requested size.
 * Adds allocation metadata before the returned buffer and optionally poisons the end.
 *
 * @param size Number of bytes to allocate
 * @return void* Pointer to the usable memory region, or NULL on failure
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

#if KMEM_DEBUG
    void *user_ptr = (void *)(allocation + 2);
    klogd("SLAB alloc(): size=%d cache=%x ptr=%x\n",
          (int)size, cache, user_ptr);
#endif

    return (void *)(allocation + 2);
}

/**
 * @brief Free a memory buffer previously allocated by alloc()
 *
 * Identifies the corresponding slab cache from the buffer’s capacity header,
 * validates poison value (if enabled), and returns the object to the cache.
 *
 * @param ptr Pointer to the memory buffer to free (NULL-safe)
 */
void free(void *ptr) {
    if (!ptr)
        return;

    uint64_t *header = (uint64_t *)ptr - 2;
    uint64_t capacity = header[0];

#if KMEM_DEBUG
    uint64_t size = header[1];
    klogd("free: Freeing a slab of size %d and capacity %d\n",
            size, capacity);
#endif

    SCACHE *cache = find_cache_from_size(capacity);
    ASSERT(capacity == CAPACITY_SIZE(cache));

#if SLAB_POISON
    uint64_t poison = *(uint64_t *)((uint8_t *)header + cache->size - sizeof(uint64_t));
    ASSERT(poison == POISON_VALUE);
#endif

    slab_free(cache, header);
}


/**
 * @brief Resize a slab-allocated buffer to a new size
 *
 * If the new size fits in the same slab cache, expands in place and zeroes new memory.
 * Otherwise, allocates a new buffer from a different cache, copies old contents,
 * and frees the old buffer.
 *
 * @param ptr Pointer to existing memory buffer (may be NULL)
 * @param new_size Desired size in bytes
 * @return void* Pointer to resized memory buffer, or NULL on failure
 */
void *realloc(void *ptr, uint64_t new_size) {
    if (!ptr)
        return alloc(new_size);

    uint64_t *header = (uint64_t *)ptr - 2;
    uint64_t old_capacity = header[0];
    uint64_t old_size     = header[1];

    SCACHE *oldc = find_cache_from_size(old_capacity);
    SCACHE *newc = find_cache_from_size(new_size);

#if KMEM_DEBUG
    klogd("SLAB realloc(): ptr=%x old_size=%d new_size=%d\n",
          ptr, (int)old_size, (int)new_size);
    klogd("  old_cache=%x new_cache=%x\n", oldc, newc);
#endif

    if (oldc == newc) {
        if (new_size > old_size) {
            memset((uint8_t *)ptr + old_size, 0, new_size - old_size);
        }
        header[1] = new_size;

#if KMEM_DEBUG
        klogd("  realloc(): in-place resize to %d bytes\n", (int)new_size);
#endif
        return ptr;
    }

    uint64_t *new_header = slab_allocate(newc);
    if (!new_header)
        return NULL;

    new_header[0] = CAPACITY_SIZE(newc);
    new_header[1] = new_size;

    memcpy(new_header + 2, ptr, MIN(old_size, CAPACITY_SIZE(newc)));

#if KMEM_DEBUG
    klogd("  realloc(): moved to new ptr=%x with cap=%d\n",
          new_header + 2, (int)CAPACITY_SIZE(newc));
#endif

    slab_free(oldc, header);

    return (void *)(new_header + 2);
}

