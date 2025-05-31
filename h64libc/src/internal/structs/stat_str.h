/**
 * @file stat_str.h
 * @author Zack Bostock
 * @brief libc file STAT structure
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#pragma once

#include <stdint.h>

#include "src/internal/structs/timespec_str.h"

typedef int64_t DEVICE;
typedef uint64_t INO;
typedef int64_t OFFSET;
typedef int64_t MODE;
typedef int64_t NLINK;
typedef int64_t BLOCK_SIZE;
typedef int64_t BLOCK_COUNT;

typedef int32_t PID;
typedef int32_t TID;
typedef int32_t UID;
typedef int32_t GID;

typedef struct {
    DEVICE dev;                     /* ID of device which has the file */
    INO ino;                        /* inode number                    */
    MODE mode;                      /* File type and mode              */
    NLINK nlink;                    /* Number of hard links to file    */
    UID uid;                        /* User ID of owner                */
    GID gid;                        /* Group ID of owner               */
    DEVICE rdev;                    /* Dev ID (if special file type)   */
    OFFSET size;                    /* Total size in bytes             */
    TIMESPEC access_time;           /* Time of last access             */
    TIMESPEC modify_time;           /* Time of last modification       */
    TIMESPEC status_change_time;    /* Time of last status change      */
    BLOCK_SIZE blksz;               /* Block size for filesystem I/O   */
    BLOCK_COUNT blk_count;          /* Number of 512B blocks alloc'd   */
} STAT;