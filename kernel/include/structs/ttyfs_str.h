/**
 * @file ttyfs_str.h
 * @author Zack Bostock
 * @brief Structures pertaining to tty filesystem
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <structs/vfs_str.h>
#include <structs/termios_str.h>

#define TTY_BUFFER_SIZE     (4096)

typedef struct {
    char name[VFS_MAX_NAME_LEN];
} TTYFS_FILE;

typedef struct {
    TTYFS_FILE entry;
    STD_TIME time;
    char name[VFS_MAX_NAME_LEN];
    VFS_INODE *parent;
} TTYFS_IDENT_ITEM;

typedef struct {
    char ibuff[TTY_BUFFER_SIZE];
    int64_t ibegin;
    int64_t icursor;
    int64_t isize;
    TERMIOS termios;
} TTYFS_IDENT;