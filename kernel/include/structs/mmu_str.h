/**
 * @file mmu_str.h
 * @author Zack Bostock
 * @brief Structs pertaining to memory management unit functionality
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>

typedef struct {
  uint64_t physical_limit;
  uint64_t total_size;
  uint64_t free_size;

  uint8_t *bitmap;
} KERNEL_MEM_INFO;

#define MMU_STATUS_LIST \
    X(MMU_SUCCESS, "sucess") \
    X(MMU_ERR_NULL_POINTER, "null_pointer") \
    X(MMU_ERR_OUT_OF_MEMORY, "out_of_memory") \
    X(MMU_ERR_INVALID_ADDRESS, "invalid_address") \
    X(MMU_ERR_NOT_MAPPED, "not_mapped") \
    X(MMU_ERR_INVALID_REQUEST, "invalid_request") \
    X(MMU_ERR_DOUBLE_FREE, "double_free")

typedef enum {
#define X(name, str) name,
    MMU_STATUS_LIST
#undef X
   MMU_ERR_NUM_TYPES
} MMU_STATUS;