/**
 * @file slab.c
 * @brief Slab allocator implementation for kernel memory manager
 */

#include <stddef.h>
#include <stdint.h>

#include <globals.h>
#include <common/lock.h>
#include <common/kprint.h>
#include <common/string.h>
#include <common/math.h>
#include <common/klib.h>
#include <sys/mmu.h>
#include <mm/slab.h>

#define SLAB_INDIRECT_CUTOFF    256U
#define SLAB_INDIRECT_COUNT     8U

#define SLAB_PAGE_OFFSET        (PAGE_SIZE - sizeof(SLAB))
#define SLAB_DATA_SIZE          SLAB_PAGE_OFFSET
#define SLAB_INDIRECT_PTR_COUNT (SLAB_DATA_SIZE / sizeof(void *))

#define GET_SLAB(ptr) ((SLAB *)(ROUND_DOWN((uintptr_t)(ptr), PAGE_SIZE) + SLAB_PAGE_OFFSET))

static uint8_t selfcache_init = FALSE;
static SCACHE selfcache = {
    .size = sizeof(SCACHE),
    .alignment = 8,
    .truesize = ROUND_UP(sizeof(SCACHE) + sizeof(void **), 8),
    .slabobjcount = SLAB_DATA_SIZE / ROUND_UP(sizeof(SCACHE)
                                              + sizeof(void **), 8)
};

static void init_direct(SCACHE *cache, SLAB *slab, void *base) {
    slab->free = NULL;
    slab->used = 0;
    slab->base = base;
    for (uint32_t offset = 0; offset < cache->truesize * cache->slabobjcount; offset += cache->truesize) {
        void *obj = (uint8_t *)base + offset;
        if (cache->constructor) {
            cache->constructor(cache, obj);
        }
        void **link = (void **)((uint8_t *)obj + cache->size);
        *link = slab->free;
        slab->free = link;
    }
}

static void init_indirect(SCACHE *cache, SLAB *slab, void *ptr, void *obj_base) {
    slab->free = NULL;
    slab->used = 0;
    slab->base = obj_base;
    for (uint32_t i = 0; i < cache->slabobjcount; ++i) {
        void *obj = (uint8_t *)obj_base + i * cache->truesize;
        if (cache->constructor) {
            cache->constructor(cache, obj);
        }
        void **link = &((void **)ptr)[i];
        *link = slab->free;
        slab->free = link;
    }
}

static uint8_t grow_cache(SCACHE *cache) {
    void *slab_mem = (void *)PHYS_TO_VIRT(pm_get(1, 0, __func__, __LINE__));
    if (!slab_mem) {
        return FALSE;
    }
    SLAB *slab = GET_SLAB(slab_mem);
    if (cache->size < SLAB_INDIRECT_CUTOFF) {
        init_direct(cache, slab, slab_mem);
    } else {
        void *obj_mem = (void *)PHYS_TO_VIRT(pm_get(
            NUM_PAGES(cache->slabobjcount * cache->truesize),
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

static void *take_object(SCACHE *cache, SLAB *slab) {
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
        return (uint8_t *)slab->base + idx * cache->truesize;
    }
}

static SLAB *return_object(SCACHE *cache, void *obj) {
    SLAB *slab = NULL;
    void **link = NULL;
    if (cache->size < SLAB_INDIRECT_CUTOFF) {
        slab = GET_SLAB(obj);
        link = (void **)((uint8_t *)obj + cache->size);
        ASSERT(*link == NULL);
    } else {
        slab = cache->partial ?: cache->full;
        /* Find correct slab */
        for (; slab; slab = slab->next) {
            uintptr_t start = (uintptr_t)slab->base;
            uintptr_t end = start + cache->slabobjcount * cache->truesize;
            if ((uintptr_t)obj >= start && (uintptr_t)obj < end) {
                break;
            }
            if (!slab->next && cache->partial) {
                slab = cache->partial;
            }
        }
        kloge("slab free: obj=%x not found in any slab (cache=%x, size=%d)\n",
            obj, cache, cache->size);

        for (SLAB *s = cache->partial; s; s = s->next) {
            uintptr_t start = (uintptr_t)s->base;
            uintptr_t end   = start + cache->slabobjcount * cache->truesize;
            klogd("slab @%x owns [%x - %x)\n", s, (void *)start, (void *)end);
        }


        ASSERT(slab);
        uint32_t idx = ((uintptr_t)obj - (uintptr_t)slab->base) / cache->truesize;
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

void *slab_allocate(SCACHE *cache) {
    LOCK_LOCK(&cache->lock);
    if (!selfcache_init) {
        selfcache.size = sizeof(SCACHE);
        selfcache.alignment = sizeof(void *);
        selfcache.truesize = ROUND_UP(selfcache.size + sizeof(void *), selfcache.alignment);
        selfcache.slabobjcount = SLAB_DATA_SIZE / selfcache.truesize;
        selfcache.constructor = NULL;
        selfcache.destructor = NULL;
        selfcache.full = selfcache.partial = selfcache.empty = NULL;
        memset(&selfcache.lock, 0, sizeof(selfcache.lock));
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
    } else if (slab->used == cache->slabobjcount) {
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
    } else if (slab->used == cache->slabobjcount - 1) {
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
 * @brief Purge up to maxcount empty slabs from a cache
 *
 * @param cache     The slab cache to purge
 * @param maxcount  Maximum number of slabs to free (use (uint64_t)-1 for all)
 * @return Number of slabs actually freed
 */
static uint64_t purge(SCACHE *cache, uint64_t maxcount) {
    uint64_t freed = 0;
    SLAB *slab = cache->empty;

    while (slab && freed < maxcount) {
        SLAB *next = slab->next;

        /* Sanity: unlink from empty list */
        if (next)
            next->prev = NULL;
        cache->empty = next;

#if USE_POISON
        /* validate poison on each object */
        if (cache->size >= SLAB_INDIRECT_CUTOFF) {
            for (uint32_t i = 0; i < cache->slabobjcount; ++i) {
                uint8_t *obj = (uint8_t *)slab->base + i * cache->truesize;
                uint64_t *poison = (uint64_t *)(obj + cache->size - sizeof(uint64_t));
                ASSERT(*poison == POISON_VALUE);
            }
        } else {
            uint8_t *base = (uint8_t *)ROUND_DOWN((uintptr_t)slab, PAGE_SIZE);
            for (uint32_t i = 0; i < cache->slabobjcount; ++i) {
                uint8_t *obj = base + i * cache->truesize;
                uint64_t *poison = (uint64_t *)(obj + cache->size - sizeof(uint64_t));
                ASSERT(*poison == POISON_VALUE);
            }
        }
#endif

        /* Free indirect slab backing storage */
        if (cache->size >= SLAB_INDIRECT_CUTOFF) {
            pm_free(VIRT_TO_PHYS(slab->base),
                    NUM_PAGES(cache->slabobjcount * cache->truesize));
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
 * @brief Free all memory associated with a slab cache
 *
 * @param cache The cache to destroy
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

    /* Return SCACHE metadata to selfcache */
    slab_free(&selfcache, cache);
}


SCACHE *slab_newcache(uint64_t size,
                        uint64_t alignment,
                        void (*constructor)(SCACHE *, void *),
                        void (*destructor)(SCACHE *, void *)) {
    if (alignment == 0) {
        alignment = 8;
    }
    if (!selfcache_init) {
        selfcache_init = TRUE;
        memset((void *) &selfcache.lock, 0, sizeof(LOCK));
    }

    SCACHE *cache = slab_allocate(&selfcache);
    if (!cache) {
        return NULL;
    }

    cache->size = size;
    cache->alignment = alignment;
    uint64_t freeptrsize = size < SLAB_INDIRECT_CUTOFF ? sizeof(void **) : 0;
    cache->truesize = ROUND_UP(size + freeptrsize, alignment);
    cache->constructor = constructor;
    cache->destructor = destructor;
    cache->slabobjcount = size < SLAB_INDIRECT_CUTOFF ?
        SLAB_DATA_SIZE / cache->truesize : SLAB_INDIRECT_COUNT;
    cache->full = NULL;
    cache->empty = NULL;
    cache->partial = NULL;

    memset((void *) &cache->lock, 0, sizeof(LOCK));

    klogd("SLAB: new cache: size %d | align %d | true size %d | obj count %d\n",
            cache->size, cache->alignment, cache->truesize, cache->slabobjcount);

    return cache;
}