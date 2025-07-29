/**
 * @file process.c
 * @author Zack Bostock
 * @brief Basic operations for creating, forking, and removing processes
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <proc/process.h>

#include <common/kprint.h>
#include <common/kmalloc.h>
#include <common/string.h>
#include <common/vector.h>
#include <common/math.h>
#include <common/hash.h>

#include <sys/cpu.h>
#include <sys/mmu.h>

static PROC_ID max_processes = DEFAULT_MAX_PROCESSES;
static PROC_ID curr_pid = 1;
static PROC_ID num_processes = 0;

/**
 * @brief Helper to get the max number of processes the system is allowed
 *
 * @return PROC_ID Max number of processes
 */
PROC_ID process_get_max_processes() {
    return max_processes;
}

/**
 * @brief Process creation function
 *
 * @param name Name of the process
 * @param entry Entry point of the process
 * @param prio Priority of the process
 * @param mode Running mode of the process
 * @param paddr_space Physical address space of the process
 * @return PROCESS* Newly created process or NULL on failure
 */
PROCESS *process_create(const char *name, void (*entry)(PROC_ID), PROC_PRIO prio,
                          PROC_MODE mode, ADDR_SPACE *paddr_space) {

    PROCESS *p = NULL;
    ADDR_SPACE *vaddr_space = NULL;
    PROC_REGS *regs = NULL;

    if (num_processes >= max_processes) {
        kloge("Process limit hit!\n");
        return NULL;
    }

    p = (PROCESS *)kmalloc(sizeof(PROCESS));
    if (!p) {
        kloge("Failed to allocate memory for a new process!\n");
        return NULL;
    }
    memset(p, 0, sizeof(PROCESS));

    p->id = curr_pid;
    p->is_forked = FALSE;

    if (mode == PROC_UMODE) {
        /* Create a new address space for a user-mode process */
        /* TODO: here is the speed culprit */
        vaddr_space = create_address_space();
        if (!vaddr_space) {
            kloge("Failed to create address space for user mode process!\n");
            goto error_cleanup;
        }

        /* Allocate the kernel stack */
        p->kstack_bottom = kcmalloc(STACK_SIZE);
        if (!p->kstack_bottom) {
            kloge("Failed to allocate space for kernel stack!\n");
            goto error_cleanup;
        }
        p->kstack_top = (void *)((uint8_t *)p->kstack_bottom + STACK_SIZE);

        /* Allocate the user stack */
        p->ustack_bottom = kcmalloc(STACK_SIZE);
        if (!p->ustack_bottom) {
            kloge("Failed to allocate space for user stack!\n");
            goto error_cleanup;
        }
        /* Convert the allocated user stack to a physical address */
        p->ustack_bottom = (void *)VIRT_TO_PHYS(p->ustack_bottom);
        p->ustack_top = (void *)((uint8_t *)p->ustack_bottom + STACK_SIZE);

        /* Set the current thread stack to the user stack */
        p->tstack_bottom = p->ustack_bottom;
        p->tstack_top = p->ustack_top;

        /* Map the user stack in both the parent physical and the new virtual address space */
        vm_map(paddr_space, (uint64_t)p->ustack_bottom,
               (uint64_t)p->ustack_bottom,
               NUM_PAGES(STACK_SIZE), VM_DEFAULT | VM_USERMODE);
        vm_map(vaddr_space, (uint64_t)p->ustack_bottom,
               (uint64_t)p->ustack_bottom,
               NUM_PAGES(STACK_SIZE), VM_DEFAULT | VM_USERMODE);

        MEM_MAP m = {
            .virt_addr = (uint64_t)p->ustack_bottom,
            .phys_addr = (uint64_t)p->ustack_bottom,
            .num_pages = NUM_PAGES(STACK_SIZE),
            .flags = VM_DEFAULT | VM_USERMODE
        };

        vector_append(&p->memmap_list, m);

        regs = (PROC_REGS *)((uint8_t *)p->ustack_top - sizeof(PROC_REGS));
        regs->cs = DEFAULT_UMODE_CODE;
        regs->ss = DEFAULT_UMODE_DATA;

        klogd("PROCESS CREATED: New user mode process (%d):\n", p->id);
        klogd("USTACK TOP: %x | USTACK BOTTOM: %x\n", p->ustack_top, p->ustack_bottom);
        klogd("KSTACK TOP: %x | KSTACK BOTTOM: %x\n", p->kstack_top, p->kstack_bottom);
    } else {
        /* Kernel-mode process: allocate only the kernel stack */
        p->kstack_bottom = kcmalloc(STACK_SIZE);
        if (!p->kstack_bottom) {
            kloge("Failed to allocate space for kernel stack!\n");
            goto error_cleanup;
        }
        p->kstack_top = (void *)((uint8_t *)p->kstack_bottom + STACK_SIZE);

        p->ustack_bottom = NULL;
        p->ustack_top = NULL;

        klogd("PROCESS CREATED: New kernel mode process (%d):\n", p->id);
        klogd("STACK TOP: %x | STACK BOTTOM: %x\n", p->kstack_top, p->kstack_bottom);

        /* Use the kernel stack as the thread stack */
        p->tstack_bottom = p->kstack_bottom;
        p->tstack_top = p->kstack_top;

        regs = (PROC_REGS *)((uint8_t *)p->kstack_top - sizeof(PROC_REGS));
        regs->cs = DEFAULT_KMODE_CODE;
        regs->ss = DEFAULT_KMODE_DATA;
    }

    p->addrspace = vaddr_space;
    regs->rsp = (uint64_t)p->tstack_top;
    regs->rflags = DEFAULT_RFLAGS;
    regs->rip = (uint64_t)entry;
    regs->rdi = curr_pid;

    p->mode = mode;
    p->tstack_top = regs;
    p->parent_id = UINT64_MAX;
    p->priority = prio;
    p->last_tick = 0;
    p->state = PROC_READY;

    strncpy(p->name, name, sizeof(p->name) - 1);
    p->name[sizeof(p->name) - 1] = '\0';

    strncpy(p->cwd, "/", sizeof(p->cwd) - 1);
    p->cwd[sizeof(p->cwd) - 1] = '\0';

    hash_init(&p->open_files);

    curr_pid++;
    num_processes++;

    if (mode == PROC_UMODE) {
        vm_unmap(paddr_space, (uint64_t)p->ustack_bottom, NUM_PAGES(STACK_SIZE));
    }

    return p;

error_cleanup:
    /* Clean up allocated resources on error */
    if (p) {
        if (p->kstack_bottom)
            kfree(p->kstack_bottom);
        if (p->ustack_bottom)
            kfree((void *)PHYS_TO_VIRT(p->ustack_bottom));
        kfree(p);
    }
    /* TODO: Need to have address space freeing function */
    return NULL;
}

/**
 * @brief Helper to duplicate memory map between two processes
 * 
 * @param parent Parent process to copy memmap from
 * @param child Child process to copy memmap to
 * @return STATUS SYS_ERR if error, SYS_OK otherwise
 */
static STATUS process_dup_memmap(PROCESS *parent, PROCESS *child) {
    if (!parent || !child) {
        kloge("Trying to duplicate memmap on NULL process!\n");
        halt();
    }
    for (size_t i = 0; i < vector_len(&parent->memmap_list); i++) {
        MEM_MAP m = vector_at(&parent->memmap_list, i);
        void *temp = kcmalloc(m.num_pages * PAGE_SIZE);
        if (!temp) {
            kloge("Failed to allocate memory during memmap duplication!\n");
            return SYS_ERR;
        }
        uint64_t ptr = VIRT_TO_PHYS(temp);
        memcpy((void *)(PHYS_TO_VIRT(ptr)), (const void *)(PHYS_TO_VIRT(m.phys_addr)),
               m.num_pages * PAGE_SIZE);
        vm_map(child->addrspace, m.virt_addr, ptr, m.num_pages, m.flags);
        m.phys_addr = ptr;
        vector_append(&child->memmap_list, m);
    }

    return SYS_OK;
}

/**
 * @brief Helper for duplicating descriptors between two processes
 * 
 * @param parent Process to copy descriptors from
 * @param child Process to copy descriptors to
 * @return STATUS SYS_ERR if error, SYS_OK otherwise
 */
STATUS process_dup_file_descriptors(PROCESS *parent, PROCESS *child,
                                    const char *func) {
    if (!parent || !child) {
        kloge("Trying to duplicate descriptors on NULL process!\n");
        halt();
    }

    for (size_t i = 0; i < parent->open_files.size; i++) {
        if (parent->open_files.entries[i].key != HASH_EMPTY_KEY &&
            parent->open_files.entries[i].data) {
            VFS_NODE_DESC *nd = (VFS_NODE_DESC *)(kmalloc(sizeof(VFS_NODE_DESC)));
            if (!nd) {
                kloge("%s: Failed to allocate memory for new VFS_NODE_DESC!\n", func);
                return SYS_ERR;
            }
            memcpy(nd, parent->open_files.entries[i].data, sizeof(VFS_NODE_DESC));
            child->open_files.entries[i].key = parent->open_files.entries[i].key;
            child->open_files.entries[i].data = nd;
            nd->inode->references++;
        }
    }

    return SYS_OK;
}

/**
 * @brief Process forking function
 *
 * @param parent Parent process to fork from
 * @return PROCESS* Newly forked child process or NULL on failure
 */
PROCESS *process_fork(PROCESS *parent) {
    if (num_processes >= max_processes) {
        kloge("Process limit hit!\n");
        return NULL;
    }

    if (parent->mode == PROC_KMODE) {
        kloge("Cannot fork kernel process!\n");
        halt();
    }

    PROCESS *child = kmalloc(sizeof(PROCESS));
    if (!child) {
        kloge("Cannot allocate memory for forked process!\n");
        return NULL;
    }

    memcpy(child, parent, sizeof(PROCESS));
    child->is_forked = TRUE;
    child->parent_id = parent->id;
    child->id = curr_pid++;
    num_processes++;

    child->addrspace = create_address_space();
    if (!child->addrspace) {
        kloge("Failed to create address space for forked process!\n");
        kfree(child);
        return NULL;
    }

    memset(&child->memmap_list, 0, sizeof(child->memmap_list));
    memset(&child->child_list, 0, sizeof(child->child_list));
    hash_init(&child->open_files);

    if (process_dup_memmap(parent, child) == SYS_ERR) {
        goto fork_error_cleanup;
    } 

    if (process_dup_file_descriptors(parent, child, __func__) == SYS_ERR) {
        goto fork_error_cleanup;
    }

    /* Duplicate the kernel stack by allocating a new one and copying content */
    child->kstack_bottom = kcmalloc(STACK_SIZE);
    if (!child->kstack_bottom) {
        kloge("Failed to allocate kernel stack for forked process!\n");
        goto fork_error_cleanup;
    }
    memcpy(child->kstack_bottom, parent->kstack_bottom, STACK_SIZE);

    /* Calculate stack for forked process */
    uint64_t offset = (uint64_t)child->kstack_top - (uint64_t)parent->kstack_bottom;
    child->kstack_top = (void *)((uint64_t)child->kstack_bottom + offset);

    /* Adjust registers on the child’s thread stack if within kernel stack range */
    if ((uint64_t)child->tstack_top >= (uint64_t)parent->kstack_bottom &&
        (uint64_t)child->tstack_top <= (uint64_t)(parent->kstack_bottom + STACK_SIZE)) {
        offset = (uint64_t)child->tstack_top - (uint64_t)parent->kstack_bottom;
        child->tstack_top = (void *)((uint64_t)child->kstack_bottom + offset);

        PROC_REGS *regs = (PROC_REGS *)(child->tstack_top);

        offset = (uint64_t)regs->rsp - (uint64_t)parent->kstack_bottom;
        regs->rsp = (uint64_t)child->kstack_bottom + offset;

        offset = (uint64_t)regs->rbp - (uint64_t)parent->kstack_bottom;
        regs->rbp = (uint64_t)child->kstack_bottom + offset;
    }

    vector_append(&parent->child_list, child->id);
    klogd("PROCESS FORKED: Parent: %d | New ID: %d\n", child->parent_id, child->id);
    return child;

fork_error_cleanup:
    /* Free resources allocated for the child in case of fork failure */
    if (child) {
        for (size_t i = 0; i < vector_len(&child->memmap_list); i++) {
            MEM_MAP m = vector_at(&child->memmap_list, i);
            vm_unmap(child->addrspace, m.virt_addr, m.num_pages);
            kfree((void *)PHYS_TO_VIRT(m.phys_addr));
        }
        /* TODO: Need to free if the vectors are allocated further up */
        // vector_free(&child->memmap_list);
        // vector_free(&child->child_list);
        // vector_free(&child->dup_list);
        if (child->kstack_bottom)
            kfree(child->kstack_bottom);
        if (child->addrspace) {
            for (size_t i = 0; i < vector_len(&child->addrspace->memory_list); i++) {
                uint64_t m = vector_at(&child->addrspace->memory_list, i);
                pm_free(m, 8);
            }
            // vector_free(&child->addrspace->memory_list);
            kfree(child->addrspace->pml4);
            kfree(child->addrspace);
        }
        kfree(child);
    }
    return NULL;
}

/**
 * @brief Function to free a process
 *
 * @param p Process to free
 */
void process_free(PROCESS *p) {
    if (!p) {
        kloge("Trying to free a NULL process!\n");
        return;
    }
    if (p->mode == PROC_KMODE) {
        kloge("Cannot free kernel process!\n");
        halt();
    }

    /* Free all memory mapping entries */
    
    /*
        Causing page fault
    */
    // for (size_t i = 0; i < vector_len(&p->memmap_list); i++) {
        // MEM_MAP m = vector_at(&p->memmap_list, i);
        // vm_unmap(p->addrspace, m.virt_addr, m.num_pages);
        /*
            TODO: Fix kfree error when trying to free certain parts
                  of memory maps.
        */
        // kcfree((void *)PHYS_TO_VIRT(m.phys_addr));
    // }
    vector_free(&p->memmap_list);
    vector_free(&p->child_list);
    vector_free(&p->dup_list);

    if (p->kstack_bottom)
        kcfree(p->kstack_bottom);

    /* Free all memory in the address space memory list */
    for (size_t i = 0; i < vector_len(&p->addrspace->memory_list); i++) {
        uint64_t m = vector_at(&p->addrspace->memory_list, i);
        pm_free(m, DEFAULT_PAGES);
    }
    vector_free(&p->addrspace->memory_list);

    klogd("FREEING PROCESS: Process ID: %d\n", p->id);

    kcfree(p->addrspace->pml4);
    kfree(p->addrspace);
    kfree(p);
}

/**
 * @brief Helper to change the name of a process 
 * 
 * @param p Process whose name to change
 * @param name Name to change to
 */
void process_change_name(PROCESS *p, const char *name) {
    if (!p) {
        kloge("Trying to change the name of a NULL process!\n");
        return;
    }

    klogd("Changing name of process %d from '%s' to '%s'\n", p->id, p->name, name);

    strncpy(p->name, name, MAX(63, strlen(name)));
}