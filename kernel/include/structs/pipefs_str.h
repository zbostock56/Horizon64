/**
 * @file pipefs_str.h
 * @author Zack Bostock
 * @brief Structures related to pipe filesystem
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <stdint.h>
#include <common/time.h>
#include <structs/vfs_str.h>

#define PIPE_BUFFER_SIZE        (4096)

typedef struct {
    char name[VFS_MAX_NAME_LEN];
} PIPEFS_FILE;

typedef struct {
    char buff[PIPE_BUFFER_SIZE];
    int64_t size;
    uint8_t closed;
} PIPEFS_IDENT;

typedef struct {
    PIPEFS_FILE entry;
    STD_TIME time;
    char name[VFS_MAX_NAME_LEN];
    VFS_INODE *parent;
} PIPEFS_IDENT_ITEM;