/**
 * @file timespec_str.h
 * @author Zack Bostock
 * @brief libc TIMESPEC struct
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <stdint.h>

typedef struct {
    int64_t TV_SEC;
    int64_t TV_NSEC;
} TIMESPEC;