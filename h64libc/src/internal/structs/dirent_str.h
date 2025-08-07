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

#include <internal/structs/stat_str.h>

#define MAX_PATH_LEN    (4096)

typedef struct {
    INO d_ino;
    OFFSET D_off;
    uint16_t d_reclen;
    uint8_t d_type;
    char d_name[MAX_PATH_LEN];
} DIRENT;