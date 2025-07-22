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
#include <sys/mmu.h>
#include <mm/slab.h>

#define SLAB_INDIRECT_CUTOFF    256U
#define SLAB_INDIRECT_COUNT     8U

#define SLAB_PAGE_OFFSET        (PAGE_SIZE - sizeof(SLAB))
#define SLAB_DATA_SIZE          SLAB_PAGE_OFFSET
#define SLAB_INDIRECT_PTR_COUNT (SLAB_DATA_SIZE / sizeof(void *))

#define GET_SLAB(ptr) ((SLAB *)(ROUND_DOWN((uintptr_t)(ptr), PAGE_SIZE) + SLAB_PAGE_OFFSET))

static uint8_t selfcache_init = FALSE;
static SCACHE selfcache;
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
    for (uint32_t offset = 0; offset < cache->truesize * cache->slabobjcount; offset += cache->truesize) {
        void *obj = (uint8_t *)base + offset;
        if (cache->ctor) cache->ctor(cache, obj);
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
        if (cache->ctor) cache->ctor(cache, obj);
        void **link = &((void **)ptr)[i];
        *link = slab->free;
        slab->free = link;
    }
}

static uint8_t growcache(SCACHE *cache) {
    void *slab_mem = (void *)PHYS_TO_VIRT(pm_get(1, 0, __func__, __LINE__));
    if (!slab_mem) return FALSE;
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

    // insert into empty list
    slab->next = cache->empty;
    slab->prev = NULL;
    if (cache->empty) cache->empty->prev = slab;
    cache->empty = slab;
    return TRUE;
}

static void *take_object(SCACHE *cache, SLAB *slab) {
    void **link = slab->free;
    if (!link) return NULL;
    slab->free = *link;
    slab->used++;
    *link = NULL;
    if (cache->size < SLAB_INDIRECT_CUTOFF) {
        return (void *)((uint8_t *)link - cache->size);
    } else {
        uintptr_t idx = ((uintptr_t)link - ROUND_DOWN((uintptr_t)slab, PAGE_SIZE)) / sizeof(void *);
        return (uint8_t *)slab->base + idx * cache->truesize;
    }
}

static SLAB *return_object(SCACHE *cache, void *obj) {
    SLAB *slab;
    void **link;
    if (cache->size < SLAB_INDIRECT_CUTOFF) {
        slab = GET_SLAB(obj);
        link = (void **)((uint8_t *)obj + cache->size);
    } else {
        slab = cache->partial ?: cache->full;
        // find correct slab
        for (; slab; slab = slab->next) {
            uintptr_t start = (uintptr_t)slab->base;
            uintptr_t end = start + cache->slabobjcount * cache->truesize;
            if ((uintptr_t)obj >= start && (uintptr_t)obj < end) break;
            if (!slab->next && cache->partial) slab = cache->partial;
        }
        uint32_t idx = ((uintptr_t)obj - (uintptr_t)slab->base) / cache->truesize;
        void **base = (void **)ROUND_DOWN((uintptr_t)slab, PAGE_SIZE);
        link = &base[idx];
    }
    if (cache->dtor) cache->dtor(cache, obj);
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
        selfcache.ctor = NULL;
        selfcache.dtor = NULL;
        selfcache.full = selfcache.partial = selfcache.empty = NULL;
        memset(&selfcache.lock, 0, sizeof(selfcache.lock));
        selfcache_init = TRUE;
    }

    // allocate cache metadata if needed
    void *ret = NULL;
    SLAB *slab = cache->partial ?: cache->empty;
    if (!slab && !growcache(cache)) goto out;
    if (!slab) slab = cache->empty;

    ret = take_object(cache, slab);

    // move slab between lists
    if (slab == cache->empty) {
        cache->empty = slab->next;
        if (cache->empty) cache->empty->prev = NULL;
        slab->next = cache->partial;
        slab->prev = NULL;
        if (cache->partial) cache->partial->prev = slab;
        cache->partial = slab;
    } else if (slab->used == cache->slabobjcount) {
        cache->partial = slab->next;
        if (cache->partial) cache->partial->prev = NULL;
        slab->next = cache->full;
        slab->prev = NULL;
        if (cache->full) cache->full->prev = slab;
        cache->full = slab;
    }
out:
    UNLOCK_LOCK(&cache->lock);
    return ret;
}

void slab_free(SCACHE *cache, void *addr) {
    if (!addr) return;
    LOCK_LOCK(&cache->lock);
    SLAB *slab = return_object(cache, addr);

    // slab empty?
    if (slab->used == 0) {
        // unlink
        if (slab->prev) slab->prev->next = slab->next; else cache->partial = slab->next;
        if (slab->next) slab->next->prev = slab->prev;
        // insert into empty
        slab->next = cache->empty;
        slab->prev = NULL;
        if (cache->empty) cache->empty->prev = slab;
        cache->empty = slab;
    }
    // slab was full, now partial?
    else if (slab->used == cache->slabobjcount - 1) {
        if (slab->prev) slab->prev->next = slab->next; else cache->full = slab->next;
        if (slab->next) slab->next->prev = slab->prev;
        // insert into partial
        slab->next = cache->partial;
        slab->prev = NULL;
        if (cache->partial) cache->partial->prev = slab;
        cache->partial = slab;
    }

    UNLOCK_LOCK(&cache->lock);
}

void slab_freecache(SCACHE *cache) {
    if (!cache) return;
    LOCK_LOCK(&cache->lock);
    // free all empty slabs
    SLAB *s = cache->empty;
    while (s) {
        SLAB *next = s->next;
        if (cache->size >= SLAB_INDIRECT_CUTOFF) {
            pm_free(VIRT_TO_PHYS(s->base),
                    NUM_PAGES(cache->slabobjcount * cache->truesize));
        }
        pm_free(VIRT_TO_PHYS(ROUND_DOWN((uintptr_t)s, PAGE_SIZE)), 1);
        s = next;
    }
    UNLOCK_LOCK(&cache->lock);
}