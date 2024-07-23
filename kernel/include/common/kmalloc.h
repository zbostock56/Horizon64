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

#include <structs/kmalloc_str.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
#define KMEM_MAGIC_NUMBER   (0xBEEFBEEF)

/* -------------------------------- GLOBALS --------------------------------- */
extern size_t kmalloc_checkno;

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void *kmalloc_impl(uint64_t size, const char *func, size_t line);
void kfree_impl(void *address, const char *func, size_t line);
void *krealloc_impl(void *address, size_t new_size, const char *func,
                    size_t line);

#define kmalloc(x)      kmalloc_impl(x, __func__, __LINE__)
#define kfree(x)        kfree_impl(x, __func__, __LINE__)
#define krealloc(x, y)     krealloc_impl(x, y, __func__, __LINE__)
