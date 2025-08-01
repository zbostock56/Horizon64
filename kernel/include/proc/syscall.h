/**
 * @file syscall.h
 * @author Zack Bostock
 * @brief Information pertaining to system calls
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>

#include <libc/errno.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
/**
 * @brief System call table
 */
#define SYSCALL_DEBUGLOG        (0)
#define SYSCALL_MMAP            (1)
#define SYSCALL_OPENAT          (2)
#define SYSCALL_READ            (3)
#define SYSCALL_WRITE           (4)
#define SYSCALL_SEEK            (5)
#define SYSCALL_CLOSE           (6)
#define SYSCALL_SET_FS_BASE     (7)
#define SYSCALL_IOCTL           (8)
#define SYSCALL_GETPID          (9)
#define SYSCALL_CHDIR           (10)
#define SYSCALL_MKDIRAT         (11)
#define SYSCALL_SOCKET          (12)
#define SYSCALL_BIND            (13)
#define SYSCALL_FORK            (14)
#define SYSCALL_EXECVE          (15)
#define SYSCALL_FACCESSAT       (16)
#define SYSCALL_FSTATAT         (17)
#define SYSCALL_FSTAT           (18)
#define SYSCALL_GETPPID         (19)
#define SYSCALL_FCNTL           (20)
#define SYSCALL_DUP3            (21)
#define SYSCALL_WAITPID         (22)
#define SYSCALL_EXIT            (23)
#define SYSCALL_READDIR         (24)
#define SYSCALL_MUNMAP          (25)
#define SYSCALL_GETCWD          (26)
#define SYSCALL_GETCLOCK        (27)
#define SYSCALL_READLINK        (28)
#define SYSCALL_GETRUSAGE       (29)
#define SYSCALL_GETRLIMIT       (30)
#define SYSCALL_UNAME           (31)
#define SYSCALL_FUTEX_WAIT      (32)
#define SYSCALL_FUTEX_WAKE      (33)
#define SYSCALL_MEMINFO         (34)
#define SYSCALL_PIPE            (35)
#define SYSCALL_UNLINK          (36)
#define SYSCALL_CHMOD           (37)
#define SYSCALL_RUNCMD          (38)
#define SYSCALL_GETENTROPY      (39)
#define SYSCALL_SIGPROCMASK     (40)
#define SYSCALL_SIGACTION       (41)

/**
 * @brief Standard I/O devices
 */
#define STDIN                   (0)
#define STDOUT                  (1)
#define STDERR                  (2)

/**
 * @brief Used in memory map of system call
 */
#define MAP_PRIVATE             (0x1)
#define MAP_SHARED              (0x2)
#define MAP_FIXED               (0x4)
#define MAP_ANONYMOUS           (0x8)

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

/**
 * @brief Represent the current working directory of file
 */
#define AT_FDCWD                (-100)

/**
 * @brief EFLAGS bits
 * @ref https://wiki.osdev.org/CPU_Registers_x86
 */
#define X86_EFLAGS_CF           (0x00000001)
#define X86_EFLAGS_PF           (0x00000004)
#define X86_EFLAGS_AF           (0x00000010)
#define X86_EFLAGS_ZF           (0x00000040)
#define X86_EFLAGS_SF           (0x00000080)
#define X86_EFLAGS_TF           (0x00000100)
#define X86_EFLAGS_IF           (0x00000200)
#define X86_EFLAGS_DF           (0x00000400)
#define X86_EFLAGS_OF           (0x00000800)
#define X86_EFLAGS_IOPL         (0x00003000)
#define X86_EFLAGS_NT           (0x00004000)
#define X86_EFLAGS_RF           (0x00010000)
#define X86_EFLAGS_VM           (0x00020000)
#define X86_EFLAGS_AC           (0x00040000)
#define X86_EFLAGS_VIF          (0x00080000)
#define X86_EFLAGS_VIP          (0x00100000)
#define X86_EFLAGS_ID           (0x00200000)

typedef int64_t (*SYSCALL_PTR)();

/**
 * @brief Flags for access() and similar system calls
 */
#define R_OK                    (4)     /* Test mode for read permission */
#define W_OK                    (2)     /* Test mode for write permission */
#define X_OK                    (1)     /* Test mode for execute permission */
#define F_OK                    (0)     /* Test mode for existance */

/**
 * @brief Symbolic link constants
 */
#define AT_SYMLINK_FOLLOW       (2)     /* Perform access checks using the    */
                                        /* effective user and group IDs       */
#define AT_EACCESS              (4)     /* If pathname is a symbolic link,    */
                                        /* do not dereference it: instead     */
                                        /* return information about the link  */
                                        /* itself.                            */

/**
 * @brief Memory protection constants
 */
#define PROT_NONE               (0)
#define PROT_READ               (1)
#define PROT_WRITE              (2)
#define PROT_EXEC               (4)


/**
 * @brief Type which indicates success status of system call
 */
typedef uint8_t SYSCALL_RETVAL;
#define SYSCALL_OK              (0)
#define SYSCALL_FAIL            (1)

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void system_calls_init();
SYSCALL_RETVAL sys_get_full_path(int64_t dirfh, const char *path, char *full_path);

/* --------------------------- EXTERNALLY DEFINED --------------------------- */
extern int64_t syscall_entry(uint64_t syscall, ...);
extern int64_t syscall_handler();
