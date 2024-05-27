#pragma once

#include <stdint.h>

typedef struct {
  uint64_t rflags;
  int lock;
} LOCK;