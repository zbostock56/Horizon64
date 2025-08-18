/**
 * @file sys.h
 * @author Zack Bostock
 * @brief Information pertaining to system call functions from the userland side
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <stddef.h>

#include "src/internal/structs/stat_str.h"
#include "src/internal/structs/dirent_str.h"

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/**
 * @brief Used as a dirfd argument for openat system call
 */
#define AT_FDCWD                (-100)

/**
 * @brief Access mode (3 bits)
 */
#define O_ACCMODE               (0x007)
#define O_EXEC                  (0x1)
#define O_RDONLY                (0x2)
#define O_RDWR                  (0x3)
#define O_SEARCH                (0x4)
#define O_WRONLY                (0x5)

/**
 * @brief Remaining file I/O bits
 */
#define O_APPEND                (0x0008)
#define O_CREAT                 (0x0010)
#define O_DIRECTORY             (0x0020)
#define O_EXCL                  (0x0040)
#define O_NOCTTY                (0x0080)
#define O_NOFOLLOW              (0x0100)
#define O_TRUNC                 (0x0200)
#define O_NONBLOCK              (0x0400)
#define O_DSYNC                 (0x0800)
#define O_RSYNC                 (0x1000)
#define O_SYNC                  (0x2000)
#define O_CLOEXEC               (0x4000)
#define O_PATH                  (0x8000)

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void libc_log(const char *msg);
void *malloc(int size);
int openat(int dirfd, const char *path, int flags);
int read(int fd, const void *buff, size_t count);
int write(int fd, const void *buff, size_t count);
int close(int fd);
int iotcl(int fd, int op, int arg);
int chdir(const char *path);
int mkdirat(const char *path);
int fork();
int execv(const char *path, char *const argv[]);
int execve(const char *path, char *const argv[], char *const envp[]);
int fstatat(int dirfd, const char *path, STAT *statbuf, int flags);
int fstat(int fd, STAT *statbuf);
int dup3(int oldfd, int newfd, int flags);
int wait(int pid);
void exit(int status);
int readdir(int fd, DIRENT *buffer);
int getcwd(char *buffer, size_t size);
int meminfo();
int pipe(int pipefd[2]);
int unlink(const char *path);
int runcmd(const char *cmd);
int open(const char *path, int flags);
void panic(const char *message);
int stat(const char *path, STAT *statbuf);