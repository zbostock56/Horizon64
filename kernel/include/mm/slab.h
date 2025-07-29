/**
 * @file slab.h
 * @author Zack Bostock
 * @brief Prototypes relevant to slabs in slab allocator
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <structs/slab_str.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void *slab_allocate(SCACHE *cache);
void slab_free(SCACHE *cache, void *addr);
SCACHE *slab_newcache(uint64_t size,
                      uint64_t alignment,
                      void (*constructor)(SCACHE *, void *),
                      void (*destructor)(SCACHE *, void *));
void slab_freecache(SCACHE *cache);
