/**
 * @file pipefs.h
 * @author Zack Bostock
 * @brief Information pertaining the pipe filesystem
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <structs/pipefs_str.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void init_pipefs();
VFS_TNODE *pipefs_open(VFS_INODE *this, const char *path);
int64_t pipefs_read(VFS_INODE *this, size_t offset, size_t len, void *buff);
int64_t pipefs_write(VFS_INODE *this, size_t offset, size_t len, const void *buff);
int64_t pipefs_mknode(VFS_TNODE *this);
int64_t pipefs_rmnode(VFS_TNODE *this);
VFS_INODE *pipefs_mount(VFS_INODE *at);