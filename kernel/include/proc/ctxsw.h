/**
 * @file ctxsw.h
 * @author Zack Bostock
 * @brief Main information of scheduler and switching process context
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <proc/process.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
/**
 * @brief These define the special values that resched.asm passes into the
 * ctxsw functions
 */
#define CTXSW_NORMAL        (0)
#define CTXSW_SLEEP         (1)
#define CTXSW_FORK          (2)

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void ctxsw(void *stack, int64_t mode);
PROC_ID sched_get_pid();
PROC_ID sched_fork();
void sched_sleep(uint64_t millis);
PROC_STATE sched_get_proc_state(PROC_ID pid);
void sched_exit(int64_t status);
uint8_t sched_resume_callback(CALLBACK cb);
CALLBACK sched_wait_callback(CALLBACK cb);
PROCESS *sched_get_curr_proc();
uint64_t sched_get_ticks();
void ctxsw_init(const char *name, uint16_t cpu_id);
uint16_t sched_get_cpu_num();
PROCESS *sched_new(const char *name, void (*entry_point)(PROC_ID), int umode);
void sched_add(PROCESS *p);
PROCESS *sched_execve(const char *path, const char *argv[], const char *envp[], const char *cwd);
__attribute__((noreturn)) void process_idle_proc(PROC_ID id);
PROCESS *sched_get_proc_by_id(PROC_ID id);