/**
 * @file slab_str.h
 * @author Zack Bostock
 * @brief Structures related to the slab allocator
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <structs/lock_str.h>

/**
 * @brief A single slab of objects within a cache.
 */
typedef struct SLAB {
    struct SLAB *next;   /**< Next slab in the list */
    struct SLAB *prev;   /**< Previous slab in the list */
    uint64_t      used;    /**< Number of objects currently allocated */
    void        **free;    /**< Head of this slab’s free‑object linked list */
    void         *base;    /**< Base address of object storage (indirect slabs) */
} SLAB;

/**
 * @brief A cache of fixed‑size objects managed by the slab allocator.
 */
typedef struct SCACHE {
    LOCK     lock;        /**< Protects all lists and counters below */
    void    (*constructor)(struct SCACHE *cache, void *obj);  /**< Called on each new object */
    void    (*destructor)(struct SCACHE *cache, void *obj);  /**< Called on each object return */
    SLAB   *full;        /**< Slabs with no free objects */
    SLAB   *partial;     /**< Slabs with some free and some used objects */
    SLAB   *empty;       /**< Slabs with all objects free */
    uint64_t  size;        /**< Client’s requested object size */
    uint64_t  truesize;    /**< size + metadata padding, rounded to alignment */
    uint64_t  alignment;   /**< Alignment requirement for each object */
    uint64_t  slabobjcount;/**< Number of objects that fit per slab page */
} SCACHE;
