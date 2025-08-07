/**
 * @file slab.c
 * @brief Slab allocator implementation for kernel memory manager
 */

#include <stddef.h>
#include <stdint.h>

#include <globals.h>
#include <common/lock.h>
#include <common/kprint.h>
#include <string.h>
#include <common/math.h>
#include <common/klib.h>
#include <sys/mmu.h>
#include <mm/slab.h>

/**
 * @brief Maximum object size (in bytes) for direct slab allocation
 *
 * Objects smaller than this use embedded object storage;
 * larger objects use indirect storage.
 */
#define SLAB_INDIRECT_CUTOFF    256U

/**
 * @brief Number of object pointers in an indirect slab
 */
#define SLAB_INDIRECT_COUNT     8U

/**
 * @brief Offset to the slab metadata within a page
 */
#define SLAB_PAGE_OFFSET        (PAGE_SIZE - sizeof(SLAB))

/**
 * @brief Size of the usable data region within a slab page
 */
#define SLAB_DATA_SIZE          SLAB_PAGE_OFFSET

/**
 * @brief Maximum number of object pointers that can fit in an indirect slab
 */
#define SLAB_INDIRECT_PTR_COUNT (SLAB_DATA_SIZE / sizeof(void *))

/**
 * @brief Computes the slab metadata pointer for a given object
 *
 * @param ptr Pointer to an object within a slab
 * @return SLAB * Pointer to the slab header
 */
#define GET_SLAB(ptr) ((SLAB *)(ROUND_DOWN((uintptr_t)(ptr), PAGE_SIZE) + \
                                                            SLAB_PAGE_OFFSET))


/**
 * @brief Initialization flag for the internal SCACHE allocator
 */
static uint8_t selfcache_init = FALSE;

/**
 * @brief Internal slab cache for allocating SCACHE structures themselves
 */
static SCACHE self_cache = {
    .size = sizeof(SCACHE),
    .alignment = 8,
    .true_size = ROUND_UP(sizeof(SCACHE) + sizeof(void **), 8),
    .slab_obj_count = SLAB_DATA_SIZE / ROUND_UP(sizeof(SCACHE) + sizeof(void **), 8)
};

/**
 * @brief Initialize a slab using direct object storage
 *
 * @param cache Cache the slab belongs to
 * @param slab Slab structure to initialize
 * @param base Base pointer to the memory page for the slab
 */
static inline void init_direct(SCACHE *cache, SLAB *slab, void *base) {
    slab->free = NULL;
    slab->used = 0;
    slab->base = base;
    for (uint32_t offset = 0; offset < cache->true_size * cache->slab_obj_count; offset += cache->true_size) {
        void *obj = (uint8_t *)base + offset;
        if (cache->constructor) {
            cache->constructor(cache, obj);
        }
        void **link = (void **)((uint8_t *)obj + cache->size);
        *link = slab->free;
        slab->free = link;
    }
}

/**
 * @brief Initialize a slab using indirect object storage
 *
 * @param cache Cache the slab belongs to
 * @param slab Slab structure to initialize
 * @param ptr Memory for the slab pointer array
 * @param obj_base Base memory for the actual objects
 */
static inline void init_indirect(SCACHE *cache, SLAB *slab, void *ptr,
                                 void *obj_base) {
    slab->free = NULL;
    slab->used = 0;
    slab->base = obj_base;
    for (uint32_t i = 0; i < cache->slab_obj_count; i++) {
        void *obj = (uint8_t *)obj_base + i * cache->true_size;
        if (cache->constructor) {
            cache->constructor(cache, obj);
        }
        void **link = &((void **)ptr)[i];
        *link = slab->free;
        slab->free = link;
    }
}

/**
 * @brief Grow a slab cache by allocating and initializing a new slab
 *
 * Allocates memory from the physical memory manager for the slab
 * (and objects, if indirect).
 *
 * @param cache The cache to grow
 * @return uint8_t TRUE on success, FALSE on failure
 */
static inline uint8_t grow_cache(SCACHE *cache) {
    void *slab_mem = (void *)PHYS_TO_VIRT(pm_get(1, 0, __func__, __LINE__));
    if (!slab_mem) {
        return FALSE;
    }
    SLAB *slab = GET_SLAB(slab_mem);
    if (cache->size < SLAB_INDIRECT_CUTOFF) {
        init_direct(cache, slab, slab_mem);
    } else {
        void *obj_mem = (void *)PHYS_TO_VIRT(pm_get(
            NUM_PAGES(cache->slab_obj_count * cache->true_size),
            0, __func__, __LINE__));
        if (!obj_mem) {
            pm_free(VIRT_TO_PHYS(slab_mem), 1);
            return FALSE;
        }
        init_indirect(cache, slab, slab_mem, obj_mem);
    }

    /* Insert into empty list */
    slab->next = cache->empty;
    slab->prev = NULL;
    if (cache->empty) {
        cache->empty->prev = slab;
    }
    cache->empty = slab;
    return TRUE;
}

/**
 * @brief Take a free object from a slab
 *
 * @param cache Cache the slab belongs to
 * @param slab Slab to allocate from
 * @return Pointer to the object, or NULL if none available
 */
static inline void *take_object(SCACHE *cache, SLAB *slab) {
    void **link = slab->free;
    if (!link) {
        return NULL;
    }
    slab->free = *link;
    slab->used++;
    *link = NULL;
    if (cache->size < SLAB_INDIRECT_CUTOFF) {
        return (void *)((uint8_t *)link - cache->size);
    } else {
        uintptr_t idx = ((uintptr_t)link -
                         ROUND_DOWN((uintptr_t)slab, PAGE_SIZE)) /
                         sizeof(void *);
        return (uint8_t *)slab->base + idx * cache->true_size;
    }
}

/**
 * @brief Return an object to its originating slab
 *
 * Performs cache destructor (if present) and reinserts the object in the
 * free list.
 *
 * @param cache Cache the object belongs to
 * @param obj Pointer to the object to return
 * @return SLAB* Pointer to the slab the object was returned to
 */
static inline SLAB *return_object(SCACHE *cache, void *obj) {
    SLAB *slab = NULL;
    void **link = NULL;

    if (cache->size < SLAB_INDIRECT_CUTOFF) {
        slab = GET_SLAB(obj);
        link = (void **)((uint8_t *)obj + cache->size);
        ASSERT(*link == NULL);
    } else {
        uintptr_t obj_addr = (uintptr_t)obj;

        for (SLAB *s = cache->partial; s; s = s->next) {
            uintptr_t start = (uintptr_t)s->base;
            uintptr_t end = start + cache->slab_obj_count * cache->true_size;
            if (obj_addr >= start && obj_addr < end) {
                slab = s;
                break;
            }
        }

        if (!slab) {
            for (SLAB *s = cache->full; s; s = s->next) {
                uintptr_t start = (uintptr_t)s->base;
                uintptr_t end = start + cache->slab_obj_count * cache->true_size;
                if (obj_addr >= start && obj_addr < end) {
                    slab = s;
                    break;
                }
            }
        }

        if (!slab) {
            kloge("SLAB return_object(): obj=%x not found in any slab!\n", obj);
        }

#if KMEM_DEBUG
        klogd("  -> cache=%x size=%d true_size=%d\n",
              cache, cache->size, cache->true_size);
        klogd("  -> inspecting slab lists:\n");

        for (SLAB *list = cache->partial; list; list = list->next) {
            uintptr_t start = (uintptr_t)list->base;
            uintptr_t end = start + cache->slab_obj_count * cache->true_size;
            klogd("    [partial] slab=%x owns [%x - %x)\n", list, start, end);
        }

        for (SLAB *list = cache->full; list; list = list->next) {
            uintptr_t start = (uintptr_t)list->base;
            uintptr_t end = start + cache->slab_obj_count * cache->true_size;
            klogd("      [full] slab=%x owns [%x - %x)\n", list, start, end);
        }

        for (SLAB *list = cache->partial; list; list = list->next) {
            uintptr_t start = (uintptr_t)list->base;
            uintptr_t end = start + cache->slab_obj_count * cache->true_size;
            if (obj_addr < start) {
                klogd("    obj is before slab=%x by %d bytes\n",
                      list, (int)(start - obj_addr));
            } else if (obj_addr >= end) {
                klogd("    obj is after slab=%x by %d bytes\n",
                      list, (int)(obj_addr - end));
            }
        }
#endif
        /* If slab is NULL, we have a serious problem */
        ASSERT(slab);
        uint32_t idx = (obj_addr - (uintptr_t)slab->base) / cache->true_size;
        void **base = (void **)ROUND_DOWN((uintptr_t)slab, PAGE_SIZE);
        link = &base[idx];
    }

    if (cache->destructor) {
        cache->destructor(cache, obj);
    }

    *link = slab->free;
    slab->free = link;
    slab->used--;
    return slab;
}



/**
 * @brief Reclaim empty slabs from a cache
 *
 * Optionally checks poison values if poisoning is enabled.
 *
 * @param cache The cache to purge
 * @param maxcount Maximum number of slabs to purge (use -1 for all)
 * @return Number of slabs successfully freed
 */
static inline uint64_t purge(SCACHE *cache, uint64_t maxcount) {
    uint64_t freed = 0;
    SLAB *slab = cache->empty;

    while (slab && freed < maxcount) {
        SLAB *next = slab->next;

        /* Sanity: unlink from empty list */
        if (next)
            next->prev = NULL;
        cache->empty = next;

#if SLAB_POISON
        /* validate poison on each object */
        if (cache->size >= SLAB_INDIRECT_CUTOFF) {
            for (uint32_t i = 0; i < cache->slab_obj_count; ++i) {
                uint8_t *obj = (uint8_t *)slab->base + i * cache->true_size;
                uint64_t *poison = (uint64_t *)(obj + cache->size - sizeof(uint64_t));
                ASSERT(*poison == POISON_VALUE);
            }
        } else {
            uint8_t *base = (uint8_t *)ROUND_DOWN((uintptr_t)slab, PAGE_SIZE);
            for (uint32_t i = 0; i < cache->slab_obj_count; ++i) {
                uint8_t *obj = base + i * cache->true_size;
                uint64_t *poison = (uint64_t *)(obj + cache->size - sizeof(uint64_t));
                ASSERT(*poison == POISON_VALUE);
            }
        }
#endif

        /* Free indirect slab backing storage */
        if (cache->size >= SLAB_INDIRECT_CUTOFF) {
            pm_free(VIRT_TO_PHYS(slab->base),
                    NUM_PAGES(cache->slab_obj_count * cache->true_size));
            slab->base = NULL;
        }

        /* Free slab itself */
        pm_free(VIRT_TO_PHYS(ROUND_DOWN((uintptr_t)slab, PAGE_SIZE)), 1);

        slab = next;
        ++freed;
    }

    return freed;
}

/**
 * @brief Allocate an object from a slab cache
 *
 * Allocates from a partially filled or empty slab, or grows the cache if needed.
 *
 * @param cache Pointer to the slab cache
 * @return void* Pointer to allocated object, or NULL on failure
 */
void *slab_allocate(SCACHE *cache) {
    LOCK_LOCK(&cache->lock);
    if (!selfcache_init) {
        self_cache.size = sizeof(SCACHE);
        self_cache.alignment = sizeof(void *);
        self_cache.true_size = ROUND_UP(self_cache.size +
                                        sizeof(void *), self_cache.alignment);
        self_cache.slab_obj_count = SLAB_DATA_SIZE / self_cache.true_size;
        self_cache.constructor = NULL;
        self_cache.destructor = NULL;
        self_cache.full = self_cache.partial = self_cache.empty = NULL;
        memset(&self_cache.lock, 0, sizeof(self_cache.lock));
        selfcache_init = TRUE;
    }

    /* Allocate cache metadata if needed */
    void *ret = NULL;
    SLAB *slab = cache->partial ?: cache->empty;
    if (!slab && !grow_cache(cache)) {
        goto out;
    }
    if (!slab) {
        slab = cache->empty;
    }

    ret = take_object(cache, slab);

    /* Move slab between lists */
    if (slab == cache->empty) {
        cache->empty = slab->next;
        if (cache->empty) {
            cache->empty->prev = NULL;
        }
        slab->next = cache->partial;
        slab->prev = NULL;
        if (cache->partial) {
            cache->partial->prev = slab;
        }
        cache->partial = slab;
    } else if (slab->used == cache->slab_obj_count) {
        cache->partial = slab->next;
        if (cache->partial) {
            cache->partial->prev = NULL;
        }
        slab->next = cache->full;
        slab->prev = NULL;
        if (cache->full) {
            cache->full->prev = slab;
        }
        cache->full = slab;
    }
out:
    UNLOCK_LOCK(&cache->lock);
    return ret;
}

/**
 * @brief Free an object back to its slab cache
 *
 * Automatically reclassifies the slab as empty, partial, or full based on usage.
 *
 * @param cache Pointer to the slab cache
 * @param addr Pointer to the object to free
 */
void slab_free(SCACHE *cache, void *addr) {
    if (!addr) {
        return;
    }

    LOCK_LOCK(&cache->lock);
    SLAB *slab = return_object(cache, addr);
    ASSERT(slab);

    /* Is slab empty? */
    if (slab->used == 0) {
        /* Unlink it */
        if (slab->prev) {
            slab->prev->next = slab->next;
        } else {
            cache->partial = slab->next;
        }

        if (slab->next) {
            slab->next->prev = slab->prev;
        }

        /* Insert into empty */
        slab->next = cache->empty;
        slab->prev = NULL;
        if (cache->empty) cache->empty->prev = slab;
        cache->empty = slab;
    } else if (slab->used == cache->slab_obj_count - 1) {
        /* Slab was full, now partial */
        if (slab->prev) {
            slab->prev->next = slab->next;
        } else {
            cache->full = slab->next;
        }
        if (slab->next) {
            slab->next->prev = slab->prev;
        }

        /* Insert into partial */
        slab->next = cache->partial;
        slab->prev = NULL;
        if (cache->partial) {
            cache->partial->prev = slab;
        }
        cache->partial = slab;
    }

    UNLOCK_LOCK(&cache->lock);
}

/**
 * @brief Free all memory and metadata associated with a slab cache
 *
 * Asserts that no live objects remain (i.e., all slabs must be empty).
 *
 * @param cache Pointer to the slab cache to destroy
 */
void slab_freecache(SCACHE *cache) {
    if (!cache)
        return;

    LOCK_LOCK(&cache->lock);

    /* Sanity: ensure no live objects remain */
    ASSERT(cache->partial == NULL);
    ASSERT(cache->full == NULL);

    /* Free all empty slabs */
    purge(cache, (uint64_t)-1);

    UNLOCK_LOCK(&cache->lock);

    /* Return SCACHE metadata to self_cache */
    slab_free(&self_cache, cache);
}

/**
 * @brief Create a new slab cache for objects of a given size
 *
 * Determines whether the cache uses direct or indirect slabs,
 * calculates alignment and true_size, and initializes metadata.
 *
 * @param size Object size in bytes
 * @param alignment Memory alignment (defaults to 8 if 0)
 * @param constructor Optional constructor function for initializing objects
 * @param destructor Optional destructor function for cleanup on free
 * @return SCACHE * Pointer to the newly created slab cache, or NULL on failure
 */
SCACHE *slab_newcache(uint64_t size,
                        uint64_t alignment,
                        void (*constructor)(SCACHE *, void *),
                        void (*destructor)(SCACHE *, void *)) {
    if (alignment == 0) {
        alignment = 8;
    }
    if (!selfcache_init) {
        selfcache_init = TRUE;
        memset((void *) &self_cache.lock, 0, sizeof(LOCK));
    }

    SCACHE *cache = slab_allocate(&self_cache);
    if (!cache) {
        return NULL;
    }

    cache->size = size;
    cache->alignment = alignment;
    uint64_t freeptrsize = size < SLAB_INDIRECT_CUTOFF ? sizeof(void **) : 0;
    cache->true_size = ROUND_UP(size + freeptrsize, alignment);
    cache->constructor = constructor;
    cache->destructor = destructor;
    cache->slab_obj_count = size < SLAB_INDIRECT_CUTOFF ?
        SLAB_DATA_SIZE / cache->true_size : SLAB_INDIRECT_COUNT;
    cache->full = NULL;
    cache->empty = NULL;
    cache->partial = NULL;

    memset((void *) &cache->lock, 0, sizeof(LOCK));

#if KMEM_DEBUG
    klogd("SLAB: new cache: size %d | align %d | true size %d | obj count %d\n",
            cache->size, cache->alignment, cache->true_size, cache->slab_obj_count);
#endif

    return cache;
}