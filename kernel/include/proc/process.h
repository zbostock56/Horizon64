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
#include <structs/gdt_str.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
/**
 * NOTE: Because of Requested Privilege Level (RPL), the usermode segments must
 *       be OR'd with 0x3 to denote RING3 operation
 */
#define DEFAULT_UMODE_DATA      (GDT_USER_DATA_64_BIT | 0x3)
#define DEFAULT_UMODE_CODE      (GDT_USER_CODE_64_BIT | 0x3)

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
void process_change_name(PROCESS *p, const char *name);
