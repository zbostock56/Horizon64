/**
 * @file stdio.h
 * @author Zack Bostock
 * @brief Horizon64's implementation of stdio.h from libc
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once


/* ---------------------------- LITERAL CONSTANTS --------------------------- */

#define STDIN       (0)
#define STDOUT      (1)
#define STDERR      (2)

#define EOF         (-1)

#include <printf.h>

#ifndef KERNEL_BUILD    /* If not compiling with the kernel */
#include <stdint.h>


#include <internal/structs/dirent_str.h>
#include <internal/structs/stat_str.h>
#include <internal/structs/timespec_str.h>

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


/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void perror(const char *s);

#else                   /* Compiling with the kernel */
#include <fs/vfs.h>
typedef VFS_STAT STAT;
#endif