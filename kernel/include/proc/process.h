/**
 * @file process.h
 * @author Zack Bostock
 * @brief Information pertaining to processes
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <structs/process_str.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
#define DEFAULT_UMODE_DATA      (0x3B)
#define DEFAULT_UMODE_CODE      (0x43)
#define DEFAULT_KMODE_DATA      (0x30)
#define DEFAULT_KMODE_CODE      (0x28)

/**
 * @ref https://wiki.osdev.org/CPU_Registers_x86-64
 */
#define DEFAULT_RFLAGS          (0x202)

#define DEFAULT_MAX_PROCESSES   (0x100)

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
PROCESS *process_create(const char *name, void (*entry)(PROC_ID), PROC_PRIO prio,
                       PROC_MODE mode, ADDR_SPACE *paddr_space);
PROCESS *process_fork(PROCESS *parent);
void process_free(PROCESS *p);
PROC_ID process_get_max_processes();
