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

#define LOCK_FN_LENGTH  (128)

typedef struct {
  uint64_t rflags;
  int line;
  int lock;
  char fn[LOCK_FN_LENGTH];
} LOCK;
