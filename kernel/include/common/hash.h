/**
 * @file hash.h
 * @author Zack Bostock
 * @brief Prototypes for generalized hashing
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>
#include <stdarg.h>

#include <structs/hash_str.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
#define HASH_EMPTY_KEY   (-1)

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void hash_init(HASH *h);
void *hash_search(HASH *h, int64_t key);
STATUS hash_insert(HASH *h, int64_t key, void *data);
void *hash_delete(HASH *h, int64_t key);
void hash_init_core(HASH *h, size_t size);