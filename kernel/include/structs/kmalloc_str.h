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
    size_t checkno;
    size_t num_pages;
    size_t size;
    char file_name[512];
    size_t lineno;
} KMEM_METADATA;
