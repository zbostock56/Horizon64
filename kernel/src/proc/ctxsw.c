/**
 * @file ctxsw.c
 * @author Zack Bostock
 * @brief Main functionality of scheduler and switching process context
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <const.h>

#include <common/kprint.h>
#include <common/lock.h>
#include <common/kmalloc.h>
#include <common/vector.h>
#include <common/hash.h>
#include <common/string.h>

#include <proc/ctxsw.h>
#include <proc/callback.h>
#include <proc/syscall.h>
#include <proc/elf.h>

#include <sys/acpi/apic.h>
#include <sys/acpi/hpet.h>
#include <sys/cpu.h>
#include <sys/smp.h>
#include <sys/tick/clkhandler.h>

#include <util/walk_memory.h>

LOCK ctxsw_lock = {0};

/**
 * @brief These are for tracking where and what processes are doing
 */
static PROCESS *proc_running[MAX_CPUS];
static PROCESS *proc_idle[MAX_CPUS];
static uint64_t proc_coords[MAX_CPUS];

static volatile uint16_t cpu_num = 0;

vector_new_static(PROCESS *, proc_active);

/**
 * @brief Assembly functions defined in resched.asm
 */
extern void exit_ctxsw(void *next_stack, uint64_t cr3val);
extern void force_ctxsw();
extern void fork_ctxsw();

/**
 * @brief Helper macro for checking if PID is valid
 */
#define CHECK_PID(proc)                                                     \
    if (proc->id < 1) {                                                     \
        kloge("SCHED: %s found a corrupted PID!\n", __func__);              \
        halt();                                                             \
    }

/**
 * @brief Main idle process that runs on the processor. Handles cleanup of
 * dead and dying processes.
 */
__attribute__((noreturn)) void process_idle_proc(PROC_ID id) {
    (void) id;

    while (TRUE) {
        /* Free resources of dead processes */
        PROCESS *p = NULL;

        LOCK_LOCK(&ctxsw_lock);
        size_t num_procs = vector_len(&proc_active);
        if (num_procs > 0) {
            /* Go through the active processes and remove those who are dead */
            for (size_t i = 0; i < num_procs; i++) {
                p = vector_at(&proc_active, i);
                if (p->state == PROC_DEAD) {
                    vector_erase(&proc_active, i);
                    break;
                } else {
                    p = NULL;
                }
            }
        }

        /* If parent processes are being killed, also kill child processes */
        if (p) {
            for (size_t i = 0; i < num_procs; i++) {
                PROCESS *parent = vector_at(&proc_active, i);
                if (p->parent_id == parent->id) {
                    for (size_t k = 0; k < vector_len(&parent->child_list); k++) {
                        PROC_ID id_child = vector_at(&parent->child_list, k);
                        if (id_child == p->id) {
                            vector_erase(&parent->child_list, k);
                            if (vector_len(&parent->child_list) == 0 &&
                                parent->state == PROC_DYING) {
                                parent->state = PROC_DEAD;
                            }
                            break;
                        }
                    }
                    break;
                }
            }
        }
        UNLOCK_LOCK(&ctxsw_lock);

        if (p) {
            /* Free all resources for this parent process */
            klogd("Idle Process: Cleaning up process (%d)\n", p->id);
            process_free(p);
        } else {
            /* Sleep processor until next interrupt if no dead tasks */
            asm volatile("hlt");
        }
    }
}

/**
 * @brief Main function where the CPU switches context to a new process
 *
 * @param stack Stack of the currently running process
 * @param mode Different type of context switch which is occuring
 */
void ctxsw(void *stack, int64_t mode) {
    /* Check to make sure that SMP has been initialized */
    if (!smp_get_info()) {
        kloge("SCHED: SMP information is NULL!\n");
        halt();
    }


    /* Kick off all callbacks that need to be processed */
    cb_dispatch();

    LOCK_LOCK(&ctxsw_lock);

    /* Get the current CPU */
    CPU *cpu = smp_get_curr_cpu(FORCE_GET_CPU);
    if (!cpu) {
        UNLOCK_LOCK(&ctxsw_lock);
        kloge("SCHED: Current CPU information is NULL!\n");
        halt();
    }

    uint16_t cpu_id = cpu->cpu_id;
    uint64_t ticks = proc_coords[cpu_id];

    PROCESS *pcurr = proc_running[cpu_id];
    PROCESS *pnext = NULL;

    /* Save the state of the process before its ctxsw'd out */
    if (pcurr) {
        pcurr->tstack_top = stack;
        pcurr->last_tick = ticks;
        pcurr->errno = cpu->errno;

        if (pcurr->state == PROC_RUNNING) {
            pcurr->state = PROC_READY;
        }

        if ((uint64_t) pcurr != (uint64_t) proc_idle[cpu_id]) {
            if (mode == CTXSW_FORK) {
                PROCESS *pfork = process_fork(pcurr);
                vector_append(&proc_active, pfork);
            }
            vector_append(&proc_active, pcurr);
        }
    } else {
        klogd("Stack was at %x, but there was not a current process\n", stack);
    }

    /* Reset the currently running process */
    proc_running[cpu_id] = NULL;
    pcurr = NULL;

    uint64_t loop_size = 0;

    /* TODO: Update to remove this garbage, it works for now. */
    while (TRUE) {
        /* Get the first process who is at the front of the active queue */
        if (vector_len(&proc_active) > 0) {
            pnext = vector_at(&proc_active, 0);
            vector_erase(&proc_active, 0);
        } else {
            pnext = NULL;
            break;
        }

        if (pnext->state == PROC_READY) {
            break;
        } else if (pnext->state == PROC_SLEEPING) {
            /* Check to see if the next process's wake up time is in the past */
            if (hpet_get_millis() >= pnext->wakeup_time &&
                pnext->wakeup_time > 0) {
                break;
            }
        }

        /* Set the pnext process to become active */
        vector_append(&proc_active, pnext);

        /* Check if the whole process list has been visited */
        if (++loop_size >= vector_len(&proc_active)) {
            pnext = NULL;
            break;
        }
    }

    if (!pnext) {
        pnext = proc_idle[cpu_id];
    }

    pnext->state = PROC_RUNNING;
    proc_running[cpu_id] = pnext;

    cpu->errno = pnext->errno;

    /* Save the current stack pointer of the kernel stack to the Task State   */
    /* Register so if a lower ring process traps, the correct stack is being  */
    /* used.                                                                  */
    cpu->tss.rsp0 = (uint64_t) pnext->kstack_top;

    proc_coords[cpu_id]++;

    UNLOCK_LOCK(&ctxsw_lock);


    if (!(cpu->tss.rsp0 & 0xFFFF000000000000) || pnext->id < 1) {
        kloge("SCHED: CPU %d kernel stack %x addrspace %x corrupted "
              "(kernel %x|%x user %x|%x in process %x pid %x, last tick %d)",
              cpu->cpu_id, cpu->tss.rsp0, pnext->addrspace, pnext->kstack_top,
              pnext->kstack_bottom, pnext->ustack_top, pnext->ustack_bottom,
              pnext, pnext->id, pnext->last_tick);
        halt();
    }

    if (pnext->fs_base != 0 && read_msr(MSR_FS_BASE_ADDR) != pnext->fs_base) {
        /* Must set the correct FS_BASE or Page Fault Exception will occur */
        write_msr(MSR_FS_BASE_ADDR, pnext->fs_base);
    }

    if (mode == CTXSW_NORMAL) {
        apic_send_end_of_interrupt();
    }

    /* Might have to reload CR3 with the newest page table */
    exit_ctxsw(pnext->tstack_top,
               (pnext->addrspace == NULL) ? 0 :
               VIRT_TO_PHYS((uint64_t) pnext->addrspace->pml4));
}

/**
 * @brief Helper function to get the process ID of the currently running process
 *
 * @return PROC_ID UINT64_MAX if error, PID otherwise
 */
PROC_ID sched_get_pid() {
    CPU *cpu = smp_get_curr_cpu(NO_FORCE_GET_CPU);
    if (!cpu) {
        return UINT64_MAX;
    }

    LOCK_LOCK(&ctxsw_lock);
    PROC_ID pid = proc_running[cpu->cpu_id]->id;
    UNLOCK_LOCK(&ctxsw_lock);

    if (pid < 1) {
        kloge("SCHED: %s returned corrupted pid!\n", __func__);
        halt();
    }

    return pid;
}

/**
 * @brief Helper to get a forked process ready to be ctxsw'd in
 *
 * @return PROC_ID Process ID of the forked process
 */
PROC_ID sched_fork() {
    CPU *cpu = smp_get_curr_cpu(NO_FORCE_GET_CPU);
    if (!cpu) {
        return UINT64_MAX;
    }

    LOCK_LOCK(&ctxsw_lock);
    PROC_ID pid = UINT64_MAX;
    PROCESS *pcurr = proc_running[cpu->cpu_id];

    if (pcurr) {
        CHECK_PID(pcurr);
        pid = pcurr->id;
    }
    UNLOCK_LOCK(&ctxsw_lock);

    force_ctxsw();

    return pid;
}

/**
 * @brief Helper functions to put process to sleep who is currently running
 *
 * @param millis Number of milliseconds to sleep
 */
void sched_sleep(uint64_t millis) {
    CPU *cpu = smp_get_curr_cpu(NO_FORCE_GET_CPU);
    if (!cpu) {
        hpet_sleep(millis);
        return;
    }

    LOCK_LOCK(&ctxsw_lock);

    PROCESS *pcurr = proc_running[cpu->cpu_id];
    if (pcurr) {
        CHECK_PID(pcurr);
        pcurr->wakeup_time = hpet_get_millis() + millis;
        pcurr->wakeup_cb.type = CB_UNDEF;
        pcurr->state = PROC_SLEEPING;
    }

    UNLOCK_LOCK(&ctxsw_lock);

    force_ctxsw();
}

/**
 * @brief Helper to get the process state of the inputted pid
 *
 * @param pid PID of process to find state of
 * @return PROC_STATE State of the process
 */
static PROC_STATE sched_get_proc_state_impl(PROC_ID pid) {
    PROCESS *proc = NULL;
    PROC_STATE state = PROC_UNKNOWN;
    uint8_t has_child = FALSE;

    /* Check in the active processes */
    for (size_t i = 0; i < vector_len(&proc_active); i++) {
        PROCESS *p = vector_at(&proc_active, i);
        if (p) {
            if (p->id == pid) {
                state = p->state;
                proc = p;
            }

            if (p->parent_id == pid) {
                if (p->state != PROC_DEAD && p->state != PROC_UNKNOWN) {
                    has_child = TRUE;
                } else if (sched_get_proc_state_impl(p->id) == PROC_RUNNING) {
                    has_child = TRUE;
                }
            }
        }
    }

    /* Check amongst all CPUs running processes */
    for (size_t i = 0; i < MAX_CPUS && !has_child; i++) {
        PROCESS *p = proc_running[i];
        if (p) {
            if (p->id == pid) {
                state = p->state;
                proc = p;
            }

            if (p->parent_id == pid) {
                if (p->state != PROC_DEAD && p->state != PROC_UNKNOWN) {
                    has_child = TRUE;
                } else if (sched_get_proc_state_impl(p->id) == PROC_RUNNING) {
                    has_child = TRUE;
                }
            }
        }
    }

    if (!has_child) {
        if (proc) {
            if (proc->state == PROC_DEAD || proc->state == PROC_DYING) {
                state = PROC_UNKNOWN;
            }
        }
    } else {
        state = PROC_RUNNING;
    }

    return state;
}

/**
 * @brief External facing helper for getting a process's state
 *
 * @param pid Process ID of the process to get the state of
 * @return PROC_STATE Process's state, if known
 */
PROC_STATE sched_get_proc_state(PROC_ID pid) {
    PROC_STATE state = PROC_UNKNOWN;

    LOCK_LOCK(&ctxsw_lock);
    state = sched_get_proc_state_impl(pid);
    UNLOCK_LOCK(&ctxsw_lock);

    return state;
}

/**
 * @brief Helper for when the scheduler is exiting
 *
 * @param status Just there for compatiblity reasons
 */
void sched_exit(int64_t status) {
    (void) status;

    CPU *cpu = smp_get_curr_cpu(NO_FORCE_GET_CPU);
    if (!cpu) {
        return;
    }

    LOCK_LOCK(&ctxsw_lock);

    PROCESS *pcurr = proc_running[cpu->cpu_id];
    if (pcurr) {
        pcurr->state = PROC_DYING;
        CHECK_PID(pcurr);
        uint8_t all_children_dead = TRUE;
        for (size_t i = 0; i < vector_len(&(pcurr->child_list)); i++) {
            if (sched_get_proc_state_impl(vector_at(&(pcurr->child_list), i))
                != PROC_DEAD) {
                all_children_dead = FALSE;
                break;
            }
        }

        /* This also works for when the current process doesn't have children */
        if (all_children_dead) {
            pcurr->state = PROC_DEAD;
        }
    }

    UNLOCK_LOCK(&ctxsw_lock);

    force_ctxsw();
}

/**
 * @brief Helper to resume callback
 *
 * @param cb Callback to resume
 * @return uint8_t True if found, false otherwise
 */
uint8_t sched_resume_callback(CALLBACK cb) {
    uint8_t ret = FALSE;

    LOCK_LOCK(&ctxsw_lock);
    for (size_t i = 0; i < vector_len(&proc_active); i++) {
        PROCESS *p = vector_at(&proc_active, i);
        if (p) {
            if (p->state == PROC_SLEEPING && p->wakeup_cb.type == cb.type) {
                p->state = PROC_READY;
                p->wakeup_cb.param = cb.param;
                ret = 1;
            }
        }
    }
    UNLOCK_LOCK(&ctxsw_lock);

    return ret;
}

/**
 * @brief Sets callback and puts process to sleep
 *
 * @param cb Callback to use when waking up
 * @return CALLBACK Callback that was set
 */
CALLBACK sched_wait_callback(CALLBACK cb) {
    CALLBACK c = {0};
    CPU *cpu = smp_get_curr_cpu(NO_FORCE_GET_CPU);
    if (!cpu) {
        return c;
    }

    LOCK_LOCK(&ctxsw_lock);
    PROCESS *pcurr = proc_running[cpu->cpu_id];
    if (pcurr) {
        CHECK_PID(pcurr);
        /* In milliseconds */
        pcurr->wakeup_time = 0;
        pcurr->wakeup_cb = cb;
        pcurr->state = PROC_SLEEPING;
    }
    UNLOCK_LOCK(&ctxsw_lock);

    force_ctxsw();

    return pcurr->wakeup_cb;
}

/**
 * @brief Helper to get the current running cpu
 *
 * @return PROCESS* Null if no cpu found, pointer to process otherwise
 */
PROCESS *sched_get_curr_proc() {
    CPU *cpu = smp_get_curr_cpu(NO_FORCE_GET_CPU);
    if (!cpu) {
        return NULL;
    }

    return proc_running[cpu->cpu_id];
}

/**
 * @brief Helper to get the number of ticks of the current running process
 *
 * @return uint64_t Number of ticks
 */
uint64_t sched_get_ticks() {
    CPU *cpu = smp_get_curr_cpu(NO_FORCE_GET_CPU);

    if (!cpu) {
        return 0;
    }

    return proc_coords[cpu->cpu_id];
}

/**
 * @brief Main initialization function for context switching and scheduling
 *
 * @param name Name of the idle process
 * @param cpu_id CPU ID to start up scheduler for
 */
void ctxsw_init(const char *name, uint16_t cpu_id) {
    klogs("INIT SCHED: starting...\n");

    /* Set the idle process */
    LOCK_LOCK(&ctxsw_lock);
    proc_idle[cpu_id] = process_create(name, process_idle_proc, 255,
                                       PROC_KMODE, NULL);
    UNLOCK_LOCK(&ctxsw_lock);

    /* Starts APIC timer and sets interrupt vector to enter_ctxsw */
    apic_timer_init();

    cpu_num++;

    klogi("INIT SCHED: Finished for CPU %d with idle task %s:%d\n", cpu_id,
          name, proc_idle[cpu_id]->id);

    klogs("INIT SCHED: finished...\n");
}

/**
 * @brief Helper to get the CPU which this scheduler is bound to
 *
 * @return uint16_t CPU number
 */
uint16_t sched_get_cpu_num() {
    return cpu_num;
}

/**
 * @brief Helper to create a new process
 *
 * @param name Name of the new process
 * @param entry_point Entry point to start execution
 * @param umode Usermode: true or kernel mode: false
 * @return PROCESS* Pointer to the new process
 */
PROCESS *sched_new(const char *name, void (*entry_point)(PROC_ID), int umode) {
    LOCK_LOCK(&ctxsw_lock);
    PROCESS *p = process_create(name, entry_point, 0, umode ? PROC_UMODE : PROC_KMODE, NULL);
    UNLOCK_LOCK(&ctxsw_lock);

    return p;
}

/**
 * @brief Helper to add a process to the active queue
 *
 * @param p Pointer to a process
 */
void sched_add(PROCESS *p) {
    LOCK_LOCK(&ctxsw_lock);
    vector_append(&proc_active, p);
    UNLOCK_LOCK(&ctxsw_lock);
}

/**
 * @brief Executes a file
 *
 * @param path Path to file
 * @param argv Command line arguments to pass to execution
 * @param envp Environment variables to pass to execution
 * @param cwd Current working directory
 * @return PROCESS* New process created to run the file, can be NULL if failure
 */
PROCESS *sched_execve(const char *path, const char *argv[], const char *envp[],
                      const char *cwd) {
    ELF_AUXVAL auxval = {0};
    uint64_t entry_point = 0;

    PROCESS *pcurr = sched_get_curr_proc();
    PROCESS *pnew = NULL;

    /* Get the name of the binary to run by grabbing the last characters */
    /* before the slash at the end of the path */
    char *pname = (char *)path;
    for (int64_t i = strlen(path) - 1; i >= 0; i--) {
        if (path[i] == '/') {
            pname = (char *) &(path[i + 1]);
            break;
        }
    }

    klogd("EXECVE: New Process (%s)\n", pname);

    LOCK_LOCK(&ctxsw_lock);

    /* Create new process in userland */
    pnew = process_create(pname, NULL, 0, PROC_UMODE, !pcurr ?
                                                       NULL : pcurr->addrspace);

    if (pcurr) {
        /* Put the current process's dup_list into the new one */
        klogd("EXECVE: Appending dup_list to new process\n");
        for (size_t i = 0; i < vector_len(&pcurr->dup_list); i++) {
            vector_append(&pnew->dup_list, vector_at(&pcurr->dup_list, i));
        }

        /* Increase the reference count of all open files */
        hash_init_core(&pnew->open_files, pcurr->open_files.size);
        for (size_t i = 0; i < pcurr->open_files.size; i++) {
            if (pcurr->open_files.entries[i].key == EMPTY_KEY ||
                !pcurr->open_files.entries[i].data) {
                continue;
            }
            VFS_NODE_DESC *nd = (VFS_NODE_DESC *) kmalloc(sizeof(VFS_NODE_DESC));
            if (!nd) {
                kloge("EXECVE: Failed to allocate more space for VFS_NODE_DESC!\n");
                halt();
            }
            memcpy(nd, pcurr->open_files.entries[i].data, sizeof(VFS_NODE_DESC));
            pnew->open_files.entries[i] = pcurr->open_files.entries[i];
            pnew->open_files.entries[i].data = nd;
            nd->inode->references++;
        }
    }

    UNLOCK_LOCK(&ctxsw_lock);

    klogd("EXEVE: Preparing to load ELF binary at (%s)\n", path);

    if (elf_load(pnew, path, &entry_point, &auxval)) {
        /* Must free memory for task now after loaded */
        process_free(pnew);
        return NULL;
    }

    PROC_REGS *pnew_regs = (PROC_REGS *)(PHYS_TO_VIRT(pnew->tstack_top));

    /* TODO: Do not check whether aux.entry == entry anymore */
    uint64_t *stack = (uint64_t *)(PHYS_TO_VIRT(pnew->tstack_top));

    if (cwd) {
        /* Copy the current working directory into the new process */
        strcpy(pnew->cwd, cwd);
    }

    uint8_t *sa = (uint8_t *)(pnew->tstack_top);
    size_t num_env = 0;
    size_t num_args = 0;

    /* --------------- Setup the stack -------------- */

    if (argv && envp) {
        size_t i = 0;
        const char *e;
        /* Copy enviroment variables to the new process */
        for (i = 0;; i++) {
            e = envp[i];
            if (!e) {
                break;
            }
            stack = (void *)stack - (strlen(e) + 1);
            strcpy((char *)stack, e);
            num_env++;
        }

        /* Copy command line arguments to the new process */
        for (i = 0;; i++) {
            e = argv[i];
            if (!e) {
                break;
            }
            stack = (void *)stack - (strlen(e) + 1);
            strcpy((char *)stack, e);
            num_args++;
        }

        /* Align stack address to 16-byte */
        stack = (void *) stack - ((uintptr_t) stack & 0xF);

        if ((num_args + num_env + 1) & 0x1) {
            stack--;
        }
    } else {
        *(--stack) = 0;
    }

    /* Set up ELF Auxilary vector */
    *(--stack) = 0;
    *(--stack) = 0;

    stack -= 2;
    stack[0] = 10;      /* AT_ENTRY */
    stack[1] = auxval.entry;

    stack -= 2;
    stack[0] = 20;      /* AT_PHDR */
    stack[1] = auxval.phdr;

    stack -= 2;
    stack[0] = 21;      /* AT_PHENT */
    stack[1] = auxval.phentsize;

    stack -= 2;
    stack[0] = 22;      /* AT_PHNUM */
    stack[1] = auxval.phnum;

    /* Environment Variables */
    *(--stack) = 0;     /* End of environment variables */

    if (argv && envp) {
        stack -= num_env;
        for (size_t i = 0; i < num_env; i++) {
            sa -= strlen(envp[i]) + 1;
            stack[i] = (uint64_t)sa;
        }
    }

    /* Command line arguments */
    *(--stack) = 0;    /* End of command line arguments */

    if (argv && envp) {
        stack -= num_args;
        for (size_t i = 0; i < num_args; i++) {
            sa -= strlen(argv[i]) + 1;
            stack[i] = (uint64_t)sa;
        }
        *(--stack) = num_args;  /* argc */
    } else {
        *(--stack) = 0;
    }

    stack = (uint64_t *)((uint64_t)stack - sizeof(PROC_REGS));
    memcpy(stack, pnew_regs, sizeof(PROC_REGS));

    pnew->tstack_top = (void *)(VIRT_TO_PHYS(stack));
    pnew_regs = (PROC_REGS *)stack;
    pnew_regs->rsp = (uint64_t)pnew->tstack_top + sizeof(PROC_REGS);

    /* --------------- Stack is now setup ----------------- */

    /* Set entry point to be the passed in function from execve */
    pnew_regs->rip = (uint64_t)entry_point;

    LOCK_LOCK(&ctxsw_lock);
    if (pcurr) {
        vector_append(&pcurr->child_list, pnew->id);
        pnew->parent_id = pcurr->id;
    }
    UNLOCK_LOCK(&ctxsw_lock);

    klogd("EXECVE: Stack is setup for new process, adding to scheduler\n");

    /* Add to scheduler to be run */
    sched_add(pnew);

    klogd("EXECVE: (%s) is now ready\n", pnew->name);

    return pnew;
}