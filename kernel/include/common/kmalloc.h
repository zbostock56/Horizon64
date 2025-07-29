/**
 * @file kmalloc.h
 * @author Zack Bostock
 * @brief Information pertaining to internal kernel memory allocator
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#include <globals.h>

#include <structs/kmalloc_str.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
#define KMEM_MAGIC_NUMBER   (0xBEEFBEEF)

/* -------------------------------- GLOBALS --------------------------------- */
#if KMEM_DEBUG
extern size_t kmalloc_checkno;
#endif

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
/* Core or slab allocator */
void *kmalloc_core(uint64_t size, const char *func, size_t line);
void kfree_core(void *address, const char *func, size_t line);
void *krealloc_core(void *address, size_t new_size, const char *func,
                    size_t line);
STATUS double_buffer_impl(void **buffer, size_t *size, size_t unit_size,
                          const char *func, size_t line);

#define kmalloc(x)          kmalloc_core(x, __func__, __LINE__)
#define kfree(x)            kfree_core(x, __func__, __LINE__)
#define krealloc(x, y)      krealloc_core(x, y, __func__, __LINE__)
#define double_buffer(buff, size, unit_size) \
    double_buffer_impl(buff, size, unit_size, __func__, __LINE__)

/* Specifically bitmap allocator */
void *kmalloc_chunk(uint64_t size, const char *func, size_t line);
void kfree_chunk(void *address, const char *func, size_t line);
void *krealloc_chunk(void *address, size_t new_size, const char *func,
                    size_t line);

#define kcmalloc(x)         kmalloc_chunk(x, __func__, __LINE__)
#define kcfree(x)           kfree_chunk(x, __func__, __LINE__)
#define kcrealloc(x, y)     krealloc_chunk(x, y, __func__, __LINE__)
