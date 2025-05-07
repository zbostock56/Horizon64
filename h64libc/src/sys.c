/**
 * @file sys.c
 * @author Zack Bostock
 * @brief Functionality for system calls on the userland side
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <libc/sys.h>

#include <stddef.h>
#include <stdint.h>

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

#define SYSCALL0(num) ({                                            \
    __asm__ volatile ("syscall"                                     \
                      : "=a" (ret), "=d" (errno)                    \
                      : "a" (num)                                   \
                      : "rcx", "r11", "memory");                    \
})

#define SYSCALL1(num, arg0) ({                                      \
    __asm__ volatile ("syscall"                                     \
                      : "=a" (ret), "=d" (errno)                    \
                      : "a" (num), "D" (arg0)                       \
                      : "rcx", "r11", "memory");                    \
})

#define SYSCALL2(num, arg0, arg1) ({                                \
    __asm__ volatile ("syscall"                                     \
                      : "=a" (ret), "=d" (errno)                    \
                      : "a" (num), "D" (arg0), "S" (arg1)           \
                      : "rcx", "r11", "memory");                    \
})

#define SYSCALL3(num, arg0, arg1, arg2) ({                          \
    __asm__ volatile ("syscall"                                     \
                      : "=a" (ret), "=d" (errno)                    \
                      : "a" (num), "D" (arg0), "S" (arg1),          \
                        "d" (arg2)                                  \
                      : "rcx", "r11", "memory");                    \
})

#define SYSCALL4(num, arg0, arg1, arg2, arg3) ({                    \
    register typeof(arg3) a3 __asm__("r10") = arg3;                 \
    __asm__ volatile ("syscall"                                     \
                      : "=a" (ret), "=d" (errno)                    \
                      : "a" (num), "D" (arg0), "S" (arg1),          \
                        "d" (arg2), "r" (a3)                        \
                      : "rcx", "r11", "memory");                    \
})

#define SYSCALL5(num, arg0, arg1, arg2, arg3, arg4) ({              \
    register typeof(arg3) a3 __asm__("r10") = arg3;                 \
    register typeof(arg4) a4 __asm__("r9") = arg4;                  \
    __asm__ volatile ("syscall"                                     \
                      : "=a" (ret), "=d" (errno)                    \
                      : "a" (num), "D" (arg0), "S" (arg1),          \
                        "d" (arg2), "r" (a3), "r" (a4)              \
                      : "rcx", "r11", "memory");                    \
})

#define SYSCALL6(num, arg0, arg1, arg2, arg3, arg4, arg5) ({        \
    register typeof(arg3) a3 __asm__("r10") = arg3;                 \
    register typeof(arg4) a4 __asm__("r9") = arg4;                  \
    register typeof(arg5) a5 __asm__("r8") = arg5;                  \
    __asm__ volatile ("syscall"                                     \
                      : "=a" (ret), "=d" (errno)                    \
                      : "a" (num), "D" (arg0), "S" (arg1),          \
                        "d" (arg2), "r" (a3), "r" (a4),             \
                        "r" (a5)                                    \
                      : "rcx", "r11", "memory");                    \
})

/* -------------------------- Numbered System Calls --------------------------*/

/**
 * @brief Calls the DEBUGLOG system call (System Call 0)
 *
 * @param msg Message to log
 */
void libc_log(const char *msg) {
    int ret, errno;
    SYSCALL1(SYSCALL_DEBUGLOG, msg);
}

/**
 * @brief Allocates memory (System Call 1)
 *
 * @param size Amount of memory to allocate
 * @return void* Pointer to start of memory section if success, NULL if failure
 */
void *malloc(int size) {
    void *ret;
    int errno;
    SYSCALL6(SYSCALL_MMAP, 0, size, 0, 0x08, 0, 0);
    return ret;
}

/**
 * @brief open a file relative to a directory file descriptor (System Call 2)
 *
 * @param dirfd Directory file descriptor (used if path is relative)
 * @param path Path to file to open
 * @param flags Describes how to open the file
 * @return int File descriptor if successful, -1 if error with errno set properly
 */
int openat(int dirfd, const char *path, int flags) {
    int ret;
    int errno;
    SYSCALL3(SYSCALL_OPENAT, dirfd, path, flags);
    return ret;
}

/**
 * @brief Read from a file (System Call 3)
 *
 * @param fd File descriptor to read from
 * @param buff Buffer to write into
 * @param count Number of bytes to read
 * @return int Bytes read if successful, -1 if failure with errno set
 */
int read(int fd, const void *buff, size_t count) {
    int64_t ret;
    int errno;
    SYSCALL3(SYSCALL_READ, fd, buff, count);
    return ret;
}

/**
 * @brief Write to a file (System Call 4)
 *
 * @param fd File descriptor to write to
 * @param buff Buffer to read from to write into fd
 * @param count Number of bytes to written
 * @return int Bytes written if successful, -1 if failure with errno set
 */
int write(int fd, const void *buff, size_t count) {
    int64_t ret;
    int errno;
    SYSCALL3(SYSCALL_WRITE, fd, buff, count);
    return ret;
}

/**
 * @brief Closes a file (System Call 6)
 *
 * @param fd File descriptor to file to close
 * @return int 0 if successful, -1 if failure with errno set
 */
int close(int fd) {
    int ret;
    int errno;
    SYSCALL1(SYSCALL_CLOSE, fd);
    return ret;
}

/**
 * @brief Change directories (System Call 10)
 *
 * @param path Directory to change to
 * @return int 0 if success, -1 if failure with errno set
 */
int chdir(const char *path) {
    int ret;
    int errno;
    SYSCALL1(SYSCALL_CHDIR, path);
    return ret;
}

/**
 * @brief Create a directory at a location (System Call 11)
 *
 * @param path Path to make the directory at
 * @return int 0 if success, -1 if failure with errno set
 */
int mkdirat(const char *path) {
    int ret;
    int errno;
    SYSCALL3(SYSCALL_MKDIRAT, AT_FDCWD, path, 0755);
    return ret;
}

/**
 * @brief Forks a process (System Call 14)
 *
 * @return int On success, the PID of the child is passed to the parent and
 * the child is passed 0. On failure, -1 is returned to the parent and no child
 * process is created with errno set appropriately.
 */
int fork() {
    int64_t ret;
    int errno;
    SYSCALL0(SYSCALL_FORK);
    return ret;
}

/**
 * @brief Execute a program (System Call 15, shared with other exec functions)
 *
 * @param path Path to executable
 * @param argv Command line arguments
 * @return int 0 if success, -1 if failure with errno set
 */
int execv(const char *path, char *const argv[]) {
    int errno;
    int ret;
    const char *envp[] = {
        "TERM=horizon",
        NULL
    };
    SYSCALL3(SYSCALL_EXECVE, path, argv, envp);
    return ret;
}

/**
 * @brief Execute a program (System Call 15, shared with other exec functions)
 *
 * @param path Path to executable
 * @param argv Command line arguments
 * @param envp Environment variables
 * @return int 0 if success, -1 if failure with errno set
 */
int execve(const char *path, char *const argv[], char *const envp[]) {
    int errno;
    int ret;
    SYSCALL3(SYSCALL_EXECVE, path, argv, envp);
    return ret;
}

/**
 * @brief Get file status (System Call 17)
 *
 * @param dirfd File descriptor of directory (used if path is relative)
 * @param path Path to file
 * @param statbuf Buffer to copy status into
 * @param flags Flags for retrieval of status
 * @return int 0 on success, -1 on failure with errno set
 */
int fstatat(int dirfd, const char *path, STAT *statbuf, int flags) {
    int errno;
    int ret;
    SYSCALL4(SYSCALL_FSTATAT, dirfd, path, statbuf, flags);
    return ret;
}

/**
 * @brief Get file status using file descriptor (System Call 18)
 *
 * @param fd File descriptor of file to get status on
 * @param statbuf Location to copy file information into
 * @return int On success 0 is returned, otherwise -1 is returned and errno is
 * set
 */
int fstat(int fd, STAT *statbuf) {
    int errno;
    int ret;
    SYSCALL2(SYSCALL_FSTAT, fd, statbuf);
    return ret;
}

/**
 * @brief Duplicate a file descriptor (System Call 21)
 *
 * @param oldfd File descriptor to copy
 * @param newfd New file descriptor
 * @param flags Flags for the new file descriptor
 * @return int Returns the new file descriptor if successful, or -1 if failure
 * with errno set
 */
int dup3(int oldfd, int newfd, int flags) {
    int errno;
    int ret;
    SYSCALL3(SYSCALL_DUP3, oldfd, newfd, flags);
    return ret;
}

/**
 * @brief Holds up a process (System call 22)
 *
 * @param pid PID to wait
 * @return int Returns 0 when finished waiting
 */
int wait(int pid) {
    while (1) {
        int errno;
        int ret;
        SYSCALL3(SYSCALL_WAITPID, pid, NULL, 0);
        if (ret < 0) {
            break;
        }
    }
    return 0;
}

/**
 * @brief Causes normal process termination (System Call 23)
 *
 * @param status Least signifcant of status is returned through here
 */
void exit(int status) {
    int ret;
    int errno;
    SYSCALL1(SYSCALL_EXIT, status);
}

/**
 * @brief Read a directory (System Call 24)
 *
 * @param fd File descriptor to directory
 * @param buffer Buffer to copy directory information into
 * @return int 0 if success, -1 if failure with errno set
 */
int readdir(int fd, DIRENT *buffer) {
    /* TODO: Update to reflect standard readdir format of DIRENT *readdir(). */
    /*       Look in man pages for more info.                                */
    int ret;
    int errno;
    SYSCALL2(SYSCALL_READDIR, fd, buffer);
    return ret;
}

/**
 * @brief Gets the current working directory and copies it back into the buffer
 * (System Call 26)
 *
 * @param buffer Buffer to copy into
 * @param size Size of buffer
 * @return int 0 if success, -1 if failure with errno set
 */
int getcwd(char *buffer, size_t size) {
    int ret;
    int errno;
    SYSCALL2(SYSCALL_GETCWD, buffer, size);
    return ret;
}

/**
 * @brief Gets current memory usage information (System Call 34)
 *
 * @return int If successful returns 0, -1 if failure with errno set properly
 */
int meminfo() {
    int64_t ret;
    int errno;
    SYSCALL0(SYSCALL_MEMINFO);
    return ret;
}

/**
 * @brief Creates a pipe, a unidirectional data channel that can be used for
 * interprocess communication (System Call 35)
 *
 * @param pipefd If successful, the two elements in the array designate two ends
 * of the pipe
 * @return int 0 if success, -1 if failure with errno set
 */
int pipe(int pipefd[2]) {
    int ret;
    int errno;
    SYSCALL1(SYSCALL_PIPE, pipefd);
    return ret;
}

/**
 * @brief Remove a link to a file and possibly delete it (System Call 36)
 *
 * @param path Path to unlink
 * @return int 0 if success, -1 if failure with errno set
 */
int unlink(const char *path) {
    int ret;
    int errno;
    SYSCALL1(SYSCALL_UNLINK, path);
    return ret;
}

/**
 * @brief Run a command in the terminal (System call 40)
 *
 * @param cmd Command to run
 * @return int 0 if success, -1 if failure with errno set
 */
int runcmd(const char *cmd) {
    int ret;
    int64_t errno;
    SYSCALL1(SYSCALL_RUNCMD, cmd);
    return ret;
}

/* ------------------------- System Call Derivatives ------------------------ */

/**
 * @brief Opens a file
 *
 * @param path Path to file to open
 * @param flags Open flags
 * @return int File descriptor if successful, -1 if failure with errno set
 */
int open(const char *path, int flags) {
    int ret = openat(AT_FDCWD, path, flags);
    return ret;
}

/**
 * @brief Panic and kill the process
 *
 * @param message Message to display
 */
void panic(const char *message) {
    int errno;
    int ret;
    SYSCALL1(SYSCALL_DEBUGLOG, message);

    /* Call exit system call on the process */
    exit(255);
}

/**
 * @brief Get file status
 *
 * @param path Path to file
 * @param statbuf Buffer to copy file status into
 * @return int 0 if success, -1 if failure with errno set
 */
int stat(const char *path, STAT *statbuf) {
    return fstatat(AT_FDCWD, path, statbuf, 0);
}