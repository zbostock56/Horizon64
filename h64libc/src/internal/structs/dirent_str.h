/**
 * @file dirent_str.h
 * @author Zack Bostock
 * @brief libc DIRENT struct
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <stdint.h>

#include "src/internal/structs/stat_str.h"

#define MAX_PATH_LEN    (4096)

typedef struct {
    INO ino;
    OFFSET off;
    uint16_t rec_len;
    uint8_t type;
    char name[MAX_PATH_LEN];
} DIRENT;