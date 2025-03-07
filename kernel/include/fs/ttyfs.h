/**
 * @file ttyfs.h
 * @author Zack Bostock
 * @brief Information pertaining the tty filesystem
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <structs/ttyfs_str.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void init_ttyfs();
int64_t ttyfs_ioctl(VFS_INODE *this, int64_t request, int64_t arg);
VFS_TNODE *ttyfs_open(VFS_INODE *this, const char *path);
int64_t ttyfs_read(VFS_INODE *this, size_t offset, size_t len, void *buff);
int64_t ttyfs_write(VFS_INODE *this, size_t offset, size_t len, const void *buff);
int64_t ttyfs_sync(VFS_INODE *this);
int64_t ttyfs_refresh(VFS_INODE *this);
int64_t ttyfs_getdent(VFS_INODE *this, size_t pos, VFS_DIR_ENTRY *dirent);
int64_t ttyfs_mknode(VFS_TNODE *this);
VFS_INODE *ttyfs_mount(VFS_INODE *at);