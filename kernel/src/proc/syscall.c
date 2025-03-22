/**
 * @file syscall.c
 * @author Zack Bostock
 * @brief Functionality pertaining to system calls
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <common/kprint.h>
#include <common/vector.h>

#include <proc/process.h>
#include <proc/ctxsw.h>
#include <proc/syscall.h>
#include <proc/callback.h>

#include <sys/smp.h>
#include <sys/cpu.h>

#include <fs/vfs.h>

#include <sys/interrupts/isr.h>

/* System call include files */
#include <proc/syscalls/debug_log.h>
#include <proc/syscalls/vm_map.h>
#include <proc/syscalls/openat.h>
#include <proc/syscalls/read.h>
#include <proc/syscalls/write.h>
#include <proc/syscalls/seek.h>
#include <proc/syscalls/close.h>
#include <proc/syscalls/set_fs_base.h>
#include <proc/syscalls/ioctl.h>
#include <proc/syscalls/getpid.h>
#include <proc/syscalls/chdir.h>
#include <proc/syscalls/mkdirat.h>
#include <proc/syscalls/fork.h>
#include <proc/syscalls/execve.h>
#include <proc/syscalls/faccessat.h>
#include <proc/syscalls/fstatat.h>
#include <proc/syscalls/fstat.h>
#include <proc/syscalls/getppid.h>
#include <proc/syscalls/fcntl.h>
#include <proc/syscalls/dup3.h>
#include <proc/syscalls/waitpid.h>
#include <proc/syscalls/exit.h>
#include <proc/syscalls/readdir.h>
#include <proc/syscalls/munmap.h>
#include <proc/syscalls/getcwd.h>
#include <proc/syscalls/getclock.h>
#include <proc/syscalls/readlink.h>
#include <proc/syscalls/getrusage.h>
#include <proc/syscalls/meminfo.h>
#include <proc/syscalls/pipe.h>
#include <proc/syscalls/unlink.h>
#include <proc/syscalls/chmod.h>
#include <proc/syscalls/runcmd.h>

/* Other non-defined system calls from standard */
int64_t sys_not_implemented();

/**
 * @brief System Call Table
 */
SYSCALL_PTR syscall_funcs[] = {
    [SYSCALL_DEBUGLOG]              = (SYSCALL_PTR)(sys_debug_log),
    [SYSCALL_MMAP]                  = (SYSCALL_PTR)(sys_vm_map),
    [SYSCALL_OPENAT]                = (SYSCALL_PTR)(sys_openat),
    [SYSCALL_READ]                  = (SYSCALL_PTR)(sys_read),
    [SYSCALL_WRITE]                 = (SYSCALL_PTR)(sys_write),
    [SYSCALL_SEEK]                  = (SYSCALL_PTR)(sys_seek),
    [SYSCALL_CLOSE]                 = (SYSCALL_PTR)(sys_close),
    [SYSCALL_SET_FS_BASE]           = (SYSCALL_PTR)(sys_set_fs_base),
    [SYSCALL_IOCTL]                 = (SYSCALL_PTR)(sys_ioctl),
    [SYSCALL_GETPID]                = (SYSCALL_PTR)(sys_getpid),
    [SYSCALL_CHDIR]                 = (SYSCALL_PTR)(sys_chdir),
    [SYSCALL_MKDIRAT]               = (SYSCALL_PTR)(sys_mkdirat),
    [SYSCALL_SOCKET]                = (SYSCALL_PTR)(sys_not_implemented),
    [SYSCALL_BIND]                  = (SYSCALL_PTR)(sys_not_implemented),
    [SYSCALL_FORK]                  = (SYSCALL_PTR)(sys_fork),
    [SYSCALL_EXECVE]                = (SYSCALL_PTR)(sys_execve),
    [SYSCALL_FACCESSAT]             = (SYSCALL_PTR)(sys_faccessat),
    [SYSCALL_FSTATAT]               = (SYSCALL_PTR)(sys_fstatat),
    [SYSCALL_FSTAT]                 = (SYSCALL_PTR)(sys_fstat),
    [SYSCALL_GETPPID]               = (SYSCALL_PTR)(sys_getppid),
    [SYSCALL_FCNTL]                 = (SYSCALL_PTR)(sys_fcntl),
    [SYSCALL_DUP3]                  = (SYSCALL_PTR)(sys_dup3),
    [SYSCALL_WAITPID]               = (SYSCALL_PTR)(sys_waitpid),
    [SYSCALL_EXIT]                  = (SYSCALL_PTR)(sys_exit),
    [SYSCALL_READDIR]               = (SYSCALL_PTR)(sys_readdir),
    [SYSCALL_MUNMAP]                = (SYSCALL_PTR)(sys_vm_unmap),
    [SYSCALL_GETCWD]                = (SYSCALL_PTR)(sys_getcwd),
    [SYSCALL_GETCLOCK]              = (SYSCALL_PTR)(sys_getclock),
    [SYSCALL_READLINK]              = (SYSCALL_PTR)(sys_readlink),
    [SYSCALL_GETRUSAGE]             = (SYSCALL_PTR)(sys_getrusage),
    [SYSCALL_GETRLIMIT]             = (SYSCALL_PTR)(sys_not_implemented),
    [SYSCALL_MEMINFO]               = (SYSCALL_PTR)(sys_meminfo),
    [SYSCALL_PIPE]                  = (SYSCALL_PTR)(sys_pipe),
    [SYSCALL_UNLINK]                = (SYSCALL_PTR)(sys_unlink),
    [SYSCALL_CHMOD]                 = (SYSCALL_PTR)(sys_chmod),
    [SYSCALL_RUNCMD]                = (SYSCALL_PTR)(sys_runcmd),
    [SYSCALL_GETENTROPY]            = (SYSCALL_PTR)(sys_not_implemented),
    [SYSCALL_SIGPROCMASK]           = (SYSCALL_PTR)(sys_not_implemented),
    [SYSCALL_SIGACTION]             = (SYSCALL_PTR)(sys_not_implemented),
};

/**
 * @brief Main initialization of system calls
 */
void system_calls_init() {
    klogs("INIT SYSTEM CALL: starting...\n");

    isr_enable_system_calls();

    /* Enable syscall */
    write_msr(MSR_EXTN_FEAT_ENABLE, read_msr(MSR_EXTN_FEAT_ENABLE) | 1);

    uint64_t star = ((uint64_t) DEFAULT_KMODE_CODE << 32) |
                    ((uint64_t) (DEFAULT_KMODE_DATA | 3) << 48);
    write_msr(MSR_STAR, star);

    /* Set where syscall will jump to */
    write_msr(MSR_LSTAR, (uint64_t) &syscall_handler);
    write_msr(MSR_SFMASK, X86_EFLAGS_TF | X86_EFLAGS_DF | X86_EFLAGS_IF |
                          X86_EFLAGS_IOPL | X86_EFLAGS_AC | X86_EFLAGS_NT);

    klogd("INIT SYSTEM CALL:\n");
    klogd("MSR_EFER: %16x\n", read_msr(MSR_EXTN_FEAT_ENABLE));
    klogd("MSR_STAR: %16x\n", read_msr(MSR_STAR));
    klogd("MSR_LSTAR: %16x\n", read_msr(MSR_LSTAR));

    klogs("INIT SYSTEM CALL: finished...\n");
}

/**
 * @brief Generic System Call to slot in for those which are not yet implemented
 *
 * @return int64_t Bogus value
 */
int64_t sys_not_implemented() {
    kloge("SYSCALL: Not implemented\n");
    halt();
    return -1;
}

/**
 * @brief Get the full path from a directory's file handle
 *
 * @param dirfh Directory's file handle
 * @param path Path to convert
 * @param full_path Buffer to copy full path into
 * @return SYSCALL_RETVAL FAIL if fail, OK if success
 */
SYSCALL_RETVAL sys_get_full_path(int64_t dirfh, const char *path,
                                 char *full_path) {
    char *fptr = full_path;

    /* Clear full_path */
    *fptr = '\0';

    /* If the directory file handle is current working directory */
    if ((int32_t)dirfh == (int32_t)VFS_FW_CWD) {
        PROCESS *pcurr = sched_get_curr_proc();
        if (!pcurr || path[0] == '/') {
            cpu_set_errno(EINVAL);
            return SYSCALL_FAIL;
        }
        /* Copy the cwd into full_path */
        strcpy(full_path, pcurr->cwd);
        /* Set pointer to end of string */
        fptr = full_path + strlen(full_path);
    } else if ((int32_t)dirfh >= 0) {
        VFS_NODE_DESC *tnode = vfs_handle_to_fd((VFS_HANDLE)dirfh);
        if (!tnode) {
            cpu_set_errno(EINVAL);
            return SYSCALL_FAIL;
        }
        /* When path starts with a dot, use the node's path */
        if (path[0] == '.') {
            strcpy(full_path, tnode->path);
            fptr = full_path + strlen(full_path);
        }
    }

    /* If path is just "." then we are done */
    if (!strcmp(path, "."))
        return SYSCALL_OK;

    /* If path starts with '/', reset full_path to root */
    if (path[0] == '/') {
        strcpy(full_path, "/");
        fptr = full_path + 1;
    }

    /* Copy path into a temporary buffer for in-place tokenization */
    char temp_path[VFS_MAX_PATH_LEN];
    strncpy(temp_path, path, sizeof(temp_path) - 1);
    temp_path[sizeof(temp_path) - 1] = '\0';

    char *token = temp_path;
    char *next;

    while (token && *token) {
        /* Find the next token delimiter */
        next = strchr(token, '/');
        if (next) {
            *next = '\0';
            next++;
        }

        if (!strcmp(token, "..")) {
            /* Back up fptr to remove the last component */
            if (fptr > full_path + 1) { /* ensure not at root */
                /* Backtrack over any non-slash characters */
                do { fptr--; } while (fptr > full_path && *(fptr - 1) != '/');
                *fptr = '\0';
            } else {
                cpu_set_errno(EINVAL);
                return SYSCALL_FAIL;
            }
        } else if (strcmp(token, ".") && token[0] != '\0') {
            /* Append separator if needed */
            if (*(fptr - 1) != '/') {
                *fptr++ = '/';
            }
            /* Copy token */
            size_t len = strlen(token);
            memcpy(fptr, token, len);
            fptr += len;
            *fptr = '\0';
        }
        token = next;
    }
    return SYSCALL_OK;
}