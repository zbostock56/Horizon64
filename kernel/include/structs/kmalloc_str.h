/**
 * @file kmalloc_str.h
 * @author Zack Bostock
 * @brief Structs pertaining to internal memory allocation
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <stddef.h>

typedef struct {
    size_t magic;
    size_t num_pages;
    size_t size;
    size_t lineno;
#if KMEM_DEBUG
    size_t checkno;
#endif
    char file_name[128];
} KMEM_METADATA;
