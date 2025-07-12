#!/bin/bash

# Paths for .c and .h files
C_DIR="kernel/src/proc/syscalls"
H_DIR="kernel/include/proc/syscalls"

# Ensure directories exist
mkdir -p "$C_DIR"
mkdir -p "$H_DIR"

# List of system call names
syscalls=("debug_log" "vm_map" "openat" "read" "write" "seek" "close" "set_fs_base" "ioctl" "getpid" "chdir" "mkdirat" "fork" "execve" "faccessat" "fstatat" "fstat" "getppid" "fnctl" "dup3" "waitpid" "exit" "readdir" "munmap" "getcwd" "getclock" "readlink" "getrusage" "meminfo" "pipe" "unlink" "chmod" "runcmd")

# Loop over each syscall name
for name in "${syscalls[@]}"; do
    # Define file paths
    c_file="$C_DIR/$name.c"
    h_file="$H_DIR/$name.h"

    # Create and write content for .c file
    cat <<EOF > "$c_file"
/**
 * @file $name.c
 * @author Zack Bostock
 * @brief Functionality pertaining to $name system call
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <proc/syscalls/$name.h>
EOF

    # Create and write content for .h file
    cat <<EOF > "$h_file"
/**
 * @file $name.h
 * @author Zack Bostock
 * @brief Functionality pertaining to $name system call
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stdint.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */

EOF

    echo "Created files: $c_file and $h_file"
done
