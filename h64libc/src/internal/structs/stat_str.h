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

#include <internal/structs/timespec_str.h>

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
    DEVICE      st_dev;         /* ID of device which has the file */
    INO         st_ino;         /* inode number                    */
    MODE        st_mode;        /* File type and mode              */
    NLINK       st_nlink;       /* Number of hard links to file    */
    UID         st_uid;         /* User ID of owner                */
    GID         st_gid;         /* Group ID of owner               */
    DEVICE      st_rdev;        /* Dev ID (if special file type)   */
    OFFSET      st_size;        /* Total size in bytes             */
    TIMESPEC    st_atim;        /* Time of last access             */
    TIMESPEC    st_mtim;        /* Time of last modification       */
    TIMESPEC    st_ctim;        /* Time of last status change      */
    BLOCK_SIZE  st_blksize;     /* Block size for filesystem I/O   */
    BLOCK_COUNT st_blocks;      /* Number of 512B blocks alloc'd   */
} STAT;