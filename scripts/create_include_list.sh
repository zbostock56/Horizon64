#!/bin/bash


syscalls=("debug_log" "vm_map" "openat" "read" "write" "seek" "close" "set_fs_base" "ioctl" "getpid" "chdir" "mkdirat" "fork" "execve" "faccessat" "fstatat" "fstat" "getppid" "fnctl" "dup3" "waitpid" "exit" "readdir" "munmap" "getcwd" "getclock" "readlink" "getrusage" "meminfo" "pipe" "unlink" "chmod" "runcmd")


for name in "${syscalls[@]}"; do
echo "#include <proc/syscalls/$name.h>"
done
