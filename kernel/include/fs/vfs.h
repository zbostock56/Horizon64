/**
 * @file vfs.h
 * @author Zack Bostock
 * @brief Virtual File System information
 * @verbatim
 * A virtual file system (VFS) or virtual filesystem switch is an abstract layer
 * on top of a more concrete file system. The purpose of a VFS is to allow client
 * applications to access different types of concrete file systems in a uniform way.
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <structs/vfs_str.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
#define VFS_FW_CWD              (-100)
#define VFS_INVALID_HANDLE      (-1)
#define VFS_MIN_HANDLE          (100)

/**
 * @brief Options for file seeking
 */
#define SEEK_CUR                (1)
#define SEEK_END                (2)
#define SEEK_SET                (3)

/**
 * @brief File types
 */
#define DT_UNKNOWN              (0)     /* Unknown file type        */
#define DT_FIFO                 (1)     /* Named pipe               */
#define DT_CHR                  (2)     /* Character device         */
#define DT_DIR                  (4)     /* A directory              */
#define DT_BLK                  (6)     /* Block device             */
#define DT_REG                  (8)     /* Regular file             */
#define DT_LNK                  (10)    /* A symbolic link          */
#define DT_SOCK                 (12)    /* A local-domain socket    */
#define DT_WHT                  (14)    /* "Whiteout" (from BSD)    */

/**
 * @brief Other modes
 */
#define S_IFMT                  (0170000)   /* Bit Mask             */
#define S_IFSOCK                (0140000)   /* Socket               */
#define S_IFLNK                 (0120000)   /* Symbolic Link        */
#define S_IFREG                 (0100000)   /* Regular file         */
#define S_IFBLK                 (0060000)   /* Block Device         */
#define S_IFDIR                 (0040000)   /* Directory            */
#define S_IFCHR                 (0020000)   /* Character Device     */
#define S_IFIFO                 (0010000)   /* FIFO Pipe            */

#define S_ISUID                 (04000)     /* Set-user-ID bit      */
#define S_ISGID                 (02000)     /* Set-group-ID bit     */
#define S_ISVTX                 (01000)     /* Sticky bit           */

/**
 * @brief Owner permissions
 */
#define S_IRWXU                 (00700)     /* Owner has RWX        */
#define S_IRUSR                 (00400)     /* Owner has read perm  */
#define S_IWUSR                 (00200)     /* Owner has write perm */
#define S_IXUSR                 (00100)     /* Owner has exec perm  */

/**
 * @brief Group permissions
 */
#define S_IRWXG                 (00070)     /* Group has RWX        */
#define S_IRGRP                 (00040)     /* Group has read perm  */
#define S_IWGRP                 (00020)     /* Group has write perm */
#define S_IXGRP                 (00010)     /* Group has exec perm  */

/**
 * @brief Other permissions
 */
#define S_IRWXO                 (00007)     /* Other has RWX        */
#define S_IROTH                 (00004)     /* Other has read perm  */
#define S_IWOTH                 (00002)     /* Other has write perm */
#define S_IXOTH                 (00001)     /* Other has exec perm  */

/**
 * @brief Modes for path to node conversion
 */
#define NO_CREATE               (0b0001U)
#define CREATE                  (0b0010U)
#define ERR_ON_EXIST            (0b0100U)

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */
#define IS_TRAVERSABLE(x) ((x)->type == VFS_DIRECTORY || (x)->type == VFS_MOUNT_POINT)

/* --------------------------- INTERNALLY DEFINED --------------------------- */
DEVICE vfs_new_dev_id();
INO vfs_new_ino_id();
VFS_FS *vfs_search_fs(char *name);
void vfs_register_fs(VFS_FS *fs);
VFS_NODE_DESC *vfs_handle_to_fd(VFS_HANDLE h);
VFS_TNODE *vfs_alloc_tnode(const char *name, VFS_INODE *inode, VFS_INODE *parent);
VFS_INODE *vfs_alloc_inode(VFS_NODE_TYPE type, uint32_t perms, uint32_t uid,
                           VFS_FS *fs, VFS_TNODE *mount_point);
STATUS vfs_free_nodes(VFS_TNODE *tnode);
VFS_NODE_DESC *vfs_handle_to_fd(VFS_HANDLE h);
VFS_TNODE *vfs_path_to_node(const char *path_name, uint8_t mode, VFS_NODE_TYPE type);
STATUS vfs_create(const char *path, VFS_NODE_TYPE type);
STATUS vfs_chmod(VFS_HANDLE h, int32_t perms);
int64_t vfs_ioctl(VFS_HANDLE h, int64_t request, int64_t arg);
STATUS vfs_mount(const char *device, char *path, char *fs_name);
int64_t vfs_tell(VFS_HANDLE h);
int64_t vfs_read(VFS_HANDLE h, size_t len, void *buff);
int64_t vfs_unlink(char *path);
int64_t vfs_write(VFS_HANDLE h, const void *buf, size_t count);
int64_t vfs_seek(VFS_HANDLE h, size_t offset, int whence);
int64_t vfs_get_parent_dir(const char *path, char *parent, char *curr_dir);
VFS_HANDLE vfs_open(const char *path, VFS_OPEN_MODE mode);
int64_t vfs_close(VFS_HANDLE h);
int64_t vfs_refresh(VFS_HANDLE h);
int64_t vfs_getdent(VFS_HANDLE h, VFS_DIR_ENTRY *dirent);
void vfs_init();