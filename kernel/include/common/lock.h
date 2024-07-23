/**
 * @file lock.h
 * @author Zack Bostock
 * @brief Information pertaining to hardware locks
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <structs/lock_str.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void lock_lock_implementation(LOCK *s, const char *f, const int ln);
void unlock_lock_implementation(LOCK *s, const char *f, const int ln);

/* --------------------------------- MACROS --------------------------------- */
#define LOCK_NEW()      (LOCK) {0, 0}
#define LOCK_LOCK(x)    lock_lock_implementation(x, __FILE__, __LINE__)
#define UNLOCK_LOCK(x)  unlock_lock_implementation(x, __FILE__, __LINE__)
