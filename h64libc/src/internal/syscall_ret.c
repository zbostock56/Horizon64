/**
 * @file syscall_ret.c
 * @author Zack Bostock
 * @brief Helper function for setting errno after a system call occurs
 * 
 * @copyright Copyright (c) 2025
 * 
 */

int __set_errno = 0;

/**
 * @brief Helper function for setting errno after a system call occurs
 * @ref https://git.musl-libc.org/cgit/musl/tree/src/internal/syscall_ret.c
 * 
 * @param r Return value from system call
 * @return long Potentially adjusted return value
 */
void __syscall_ret(int r) {
    __set_errno = r;
}