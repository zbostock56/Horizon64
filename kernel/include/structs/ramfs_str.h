/**
 * @file ramfs_str.h
 * @author Zack Bostock
 * @brief Structures related to ram filesystem
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <stdint.h>
#include <structs/vfs_str.h>

typedef struct {
    char name[VFS_MAX_NAME_LEN];
    void *data;
    uint64_t size;
} RAMFS_FILE;

/**
 * @brief POSIX complient version of TAR (Tape Archive Format)
 * @ref https://wiki.osdev.org/USTAR
 */
typedef struct {
    char name[100];                 /* File name */
    uint64_t mode;                  /* File Mode */
    uint64_t owner_id;              /* Owner's numeric user ID */
    uint64_t group_id;              /* Group's numeric user ID */
    uint8_t size[12];               /* File size in bytes (octal base) */
    uint8_t last_modified[12];      /* Last modification time in numeric Unix time format (octal) */
    uint64_t checksum;              /* Checksum for header record */
    uint8_t type;                   /* type flag */
    uint8_t linked_file_name[100];  /* Name of linked file */
    uint8_t indicator;              /* UStar indiciator "ustar" then NUL */
    uint8_t version[2];             /* UStar version "00" */
    uint8_t owner_user_name[32];    /* Owner user name */
    uint8_t owner_group_name[32];   /* Owner group name */
    uint64_t dev_major_number;      /* Device major number */
    uint64_t dev_minor_number;      /* Device minor number */
    uint8_t filename_prefix[155];   /* Filename prefix */
} __attribute__((packed)) USTAR_FILE;

typedef struct {
    RAMFS_FILE entry;
    VFS_NODE_TYPE type;
    STD_TIME time;
    char name[VFS_MAX_NAME_LEN];
    char path[VFS_MAX_PATH_LEN];
    VFS_INODE *parent;
} RAMFS_IDENT_ITEM;

typedef struct {
    size_t alloc_size;
    void *data;
} RAMFS_IDENT;