/**
 * @file hash_str.h
 * @author Zack Bostock
 * @brief Structures related to hashing
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

typedef struct {
    int64_t key;
    void *data;
} HASH_ENTRY;

typedef struct {
    size_t size;
    HASH_ENTRY *entries;
} HASH;