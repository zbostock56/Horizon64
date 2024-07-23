/**
 * @file lock_str.h
 * @author Zack Bostock
 * @brief Structs pertaining to hardware locking functionality
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>

typedef struct {
  uint64_t rflags;
  int lock;
} LOCK;
