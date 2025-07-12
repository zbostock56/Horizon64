/**
 * @file process_str.h
 * @author Zack bostock
 * @brief Structures related to a schedulable process
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <structs/time_str.h>

#include <common/vector.h>
#include <common/hash.h>

#include <sys/mmu.h>

#include <fs/vfs.h>

typedef uint64_t PROC_ID;
typedef uint8_t PROC_PRIO;

/**
 * @brief Specifies if the process is running in kernel mode or user mode
 */
typedef enum {
    PROC_KMODE,
    PROC_UMODE
} PROC_MODE;

/**
 * @brief Defines the current status of the process
 */
typedef enum {
    PROC_READY,
    PROC_RUNNING,
    PROC_SLEEPING,
    PROC_DYING,
    PROC_DEAD,
    PROC_UNKNOWN
} PROC_STATE;

typedef struct {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed)) PROC_REGS;

/**
 * @brief Specifies the type of callback
 */
typedef enum {
    CB_UNDEF = 1,
    CB_KEY_PRESS
} CB_TYPE;

typedef uint64_t CB_PARAM;

typedef struct {
    PROC_ID publisher_id;
    PROC_ID subscriber_id;
    CB_TYPE type;
    CB_PARAM param;
    uint64_t timestamp;
} CALLBACK;

typedef struct {
    VFS_HANDLE prev;
    VFS_HANDLE new;
} FILE_DUP;

/**
 * @brief Information about passable to user space when ELF binary loader goes
 * to load a program into memory
 */
typedef struct {
    uint64_t entry;
    uint64_t phdr;
    uint64_t phaddr;
    uint16_t phentsize;
    uint16_t phnum;
    uint64_t shdr;
    uint16_t shnum;
} __attribute__((packed)) ELF_AUXVAL;

/**
 * @brief General process structure
 */
typedef struct {
    /* Stack information */
    void *tstack_top;
    void *tstack_bottom;
    void *kstack_top;
    void *kstack_bottom;
    void *ustack_top;
    void *ustack_bottom;

    /* Process metadata */
    PROC_ID id;
    PROC_ID parent_id;
    PROC_PRIO priority;
    uint64_t last_tick;
    uint64_t wakeup_time;
    CALLBACK wakeup_cb;
    PROC_STATE state;
    PROC_MODE mode;
    uint8_t is_forked;

    /* Loader information */
    ELF_AUXVAL aux;

    /* child processes */
    vector_struct(PROC_ID) child_list;

    /* File descriptor information */
    HASH open_files;
    vector_struct(FILE_DUP) dup_list;

    /* Process error */
    int64_t errno;

    /* Memory information */
    ADDR_SPACE *addrspace;
    vector_struct(MEM_MAP) memmap_list;
    uint64_t fs_base;

    char cwd[VFS_MAX_PATH_LEN];
    char name[64];
} PROCESS;