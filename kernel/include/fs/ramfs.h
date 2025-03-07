/**
 * @file ramfs.h
 * @author Zack Bostock
 * @brief Information pertaining to ram filesystem
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <structs/ramfs_str.h>
#include <structs/vfs_str.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void init_ramfs(void *addr, uint64_t size);
VFS_TNODE *ramfs_open(VFS_INODE *this, const char *path);
int64_t ramfs_read(VFS_INODE *this, size_t offset, size_t len, void *buff);
int64_t ramfs_rmnode(VFS_TNODE *this);
int64_t ramfs_write(VFS_INODE *this, size_t offset, size_t len, const void *buff);
int64_t ramfs_sync(VFS_INODE *this);
int64_t ramfs_setlink(VFS_TNODE *this, VFS_INODE *inode);
int64_t ramfs_refresh(VFS_INODE *this);
int64_t ramfs_getdent(VFS_INODE *this, size_t pos, VFS_DIR_ENTRY *dirent);
int64_t ramfs_mknode(VFS_TNODE *this);
VFS_INODE *ramfs_mount(VFS_INODE *at);