/**-----------------------------------------------------------------------------
 * @file    alloc.c
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
 **----------------------------------------------------------------------------*/
#include <stddef.h>
#include <stdint.h>

#include <globals.h>
#include <common/kprint.h>
#include <common/klib.h>
#include <sys/asm.h>
#include <libc/string.h>
#include <mm/slab.h>
#include <mm/alloc.h>

#define USE_POISON      0
#define POISON_VALUE    0xdeadbeefbadc0ffel
#define CACHE_COUNT     12

#define CAPACITY_SIZE(cache)    ((cache)->size - sizeof(uint64_t)*2 - USE_POISON*sizeof(uint64_t))

static uint64_t allocsizes[CACHE_COUNT] = {
    32, 64, 128, 256, 512, 1024, 2048, 4096,
    8192, 16384, 32768, ALLOC_MAX_SIZE
};

static SCACHE *caches[CACHE_COUNT];

static void initarea(SCACHE *cache, void *obj) {
    uint64_t *header = obj;
    *header = CAPACITY_SIZE(cache);
    /* zero data region */
    memset(header + 2, 0, *header);
#if USE_POISON
    *((uint64_t *)((uint8_t *)obj + cache->size - sizeof(uint64_t))) = POISON_VALUE;
#endif
}

static void dtor(SCACHE *cache, void *obj) {
#if USE_POISON
    uint64_t poison = *(uint64_t *)((uint8_t *)obj + cache->size - sizeof(uint64_t));
    ASSERT(poison == POISON_VALUE);
#endif
    initarea(cache, obj);
}

static SCACHE *getcachefromsize(uint64_t size) {
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
    for (int i = 0; i < CACHE_COUNT; ++i) {
        uint64_t obj_size = allocsizes[i] + sizeof(uint64_t)*2 + USE_POISON*sizeof(uint64_t);
        caches[i] = slab_newcache(obj_size, 0, initarea, dtor);
        ASSERT(caches[i]);
    }
    klogi("alloc: initialized %d slab caches", CACHE_COUNT);
}

/**
 * @brief Allocate size bytes from appropriate slab cache
 */
void *alloc(uint64_t size) {
    SCACHE *cache = getcachefromsize(size);
    uint64_t *allocation = slab_allocate(cache);
    if (!allocation)
        return NULL;
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
    uint64_t capacity = *header;
    SCACHE *cache = getcachefromsize(capacity);
    slab_free(cache, header);
}

/**
 * @brief Reallocate a slab-allocated pointer to new size
 */
void *realloc(void *ptr, uint64_t new_size) {
    if (!ptr)
        return alloc(new_size);

    uint64_t *header = (uint64_t *)ptr - 2;
    uint64_t curr_size = *(header + 1);
    if (new_size <= curr_size) {
        /* shrink in place */
        *(header + 1) = new_size;
        return ptr;
    }

    SCACHE *oldc = getcachefromsize(*header);
    SCACHE *newc = getcachefromsize(new_size);
    if (oldc == newc) {
        /* same cache, expand data region */
        memset((uint8_t *)ptr + curr_size, 0, new_size - curr_size);
        *(header + 1) = new_size;
        return ptr;
    }

    /* move to different cache */
    uint64_t *new_header = slab_allocate(newc);
    if (!new_header)
        return NULL;
    * (new_header + 1) = new_size;
    memcpy(new_header + 2, ptr, curr_size);
    slab_free(oldc, header);
    return (void *)(new_header + 2);
}
