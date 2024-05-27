#pragma once

#include <stdint.h>

typedef struct {
  uint64_t physical_limit;
  uint64_t total_size;
  uint64_t free_size;

  uint8_t *bitmap;
} KERNEL_MEM_INFO;