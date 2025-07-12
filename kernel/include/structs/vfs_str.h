/**
 * @file vfs_str.h
 * @author Zack Bostock
 * @brief Structs associated with the virtual file system driver
 * @verbatim
 * A virtual file system (VFS) or virtual filesystem switch is an abstract layer
 * on top of a more concrete file system. The purpose of a VFS is to allow client
 * applications to access different types of concrete file systems in a uniform way.
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <stdint.h>

#include <structs/time_str.h>

#include <common/vector.h>

/**
 * @brief Limits on specific file system ideas
 */
#define VFS_MAX_PATH_LEN        (4096)
#define VFS_MAX_NAME_LEN        (256)

/**
 * @brief VFS related types
 */
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

/**
 * @brief VFS data structure types
 */
typedef int64_t VFS_HANDLE;
typedef struct VFS_INODE VFS_INODE;
typedef struct VFS_TNODE VFS_TNODE;

typedef struct {
    INO ino;
    OFFSET off;
    uint16_t rec_len;
    uint8_t type;
    char name[VFS_MAX_PATH_LEN];
} DIRENT;

typedef enum {
    VFS_FILE,
    VFS_SYMLINK,
    VFS_DIRECTORY,
    VFS_BLOCK_DEV,
    VFS_CHAR_DEV,
    VFS_MOUNT_POINT,
    VFS_INVALID
} VFS_NODE_TYPE;

typedef enum {
    VFS_READ,
    VFS_WRITE,
    VFS_READ_WRITE
} VFS_OPEN_MODE;

typedef struct {
    int64_t TV_SEC;
    int64_t TV_NSEC;
} VFS_TIMESPEC;

typedef struct {
    DEVICE dev;                     /* ID of device which has the file */
    INO ino;                        /* inode number                    */
    MODE mode;                      /* File type and mode              */
    NLINK nlink;                    /* Number of hard links to file    */
    UID uid;                        /* User ID of owner                */
    GID gid;                        /* Group ID of owner               */
    DEVICE rdev;                    /* Dev ID (if special file type)   */
    OFFSET size;                    /* Total size in bytes             */
    VFS_TIMESPEC access_time;       /* Time of last access             */
    VFS_TIMESPEC modify_time;       /* Time of last modification       */
    VFS_TIMESPEC status_change_time;/* Time of last status change      */
    BLOCK_SIZE blksz;               /* Block size for filesystem I/O   */
    BLOCK_COUNT blk_count;          /* Number of 512B blocks alloc'd   */
} VFS_STAT;

typedef struct {
    VFS_NODE_TYPE type;
    STD_TIME time;
    char name[VFS_MAX_PATH_LEN];
    size_t size;
} VFS_DIR_ENTRY;

typedef struct {
    char name[16];
    uint8_t is_temp; /* False for ramfs, true otherwise */
    vector_struct(void *) files;

    VFS_INODE *(*mount)(VFS_INODE *device);
    VFS_TNODE *(*open)(VFS_INODE *this, const char *path);
    int64_t (*mknode)(VFS_TNODE *this);
    int64_t (*rmnode)(VFS_TNODE *this);
    int64_t (*read)(VFS_INODE *this, size_t offset, size_t len, void *buff);
    int64_t (*write)(VFS_INODE *this, size_t offset, size_t len, const void *buff);
    int64_t (*sync)(VFS_INODE *this);
    int64_t (*refresh)(VFS_INODE *this);
    int64_t (*getdent)(VFS_INODE *this, size_t pos, VFS_DIR_ENTRY *dir);
    int64_t (*ioctl)(VFS_INODE *this, int64_t req, int64_t arg);
} VFS_FS;

struct VFS_TNODE {
    char name[VFS_MAX_NAME_LEN];
    VFS_STAT stat;
    VFS_INODE *inode;
    VFS_INODE *parent;
};

struct VFS_INODE {
    VFS_NODE_TYPE type;                 /* File type */
    char symlink[VFS_MAX_NAME_LEN];     /* Target file if file is symlink */
    size_t size;                        /* File size */
    uint32_t permissions;               /* File permissions modifiable by chmod */
    uint32_t uid;                       /* User ID */
    uint32_t references;                /* Reference Count, used by symlinks */
    STD_TIME time;                      /* Time informaiton */
    VFS_FS *fs;                         /* File system correlated with */
    void *ident;                        /* Identifier */
    VFS_TNODE *mount_point;             /* Mount location */
    vector_struct(VFS_TNODE *) child;   /* Vector of children */
};

typedef struct {
    char path[VFS_MAX_NAME_LEN];
    VFS_TNODE *tnode;
    VFS_INODE *inode;
    VFS_OPEN_MODE mode;
    size_t seek_position;
    VFS_TNODE *current_dir_ent;
    size_t current_dir_index;
} VFS_NODE_DESC;