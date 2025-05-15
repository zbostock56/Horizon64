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

    /* Set the process ID */
    p->id = curr_pid;
    /* Mark this as a new (non-forked) process */
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
        p->kstack_bottom = kmalloc(STACK_SIZE);
        if (!p->kstack_bottom) {
            kloge("Failed to allocate space for kernel stack!\n");
            goto error_cleanup;
        }
        p->kstack_top = (void *)((uint8_t *)p->kstack_bottom + STACK_SIZE);

        /* Allocate the user stack */
        p->ustack_bottom = kmalloc(STACK_SIZE);
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
        vm_map(paddr_space, (uint64_t)p->ustack_bottom, (uint64_t)p->ustack_bottom,
               NUM_PAGES(STACK_SIZE), VM_DEFAULT | VM_USERMODE);
        vm_map(vaddr_space, (uint64_t)p->ustack_bottom, (uint64_t)p->ustack_bottom,
               NUM_PAGES(STACK_SIZE), VM_DEFAULT | VM_USERMODE);

        MEM_MAP m = {
            .virt_addr = (uint64_t)p->ustack_bottom,
            .phys_addr = (uint64_t)p->ustack_bottom,
            .num_pages = NUM_PAGES(STACK_SIZE),
            .flags = VM_DEFAULT | VM_USERMODE
        };

        vector_append(&p->memmap_list, m);

        /* Calculate register area using proper byte pointer arithmetic */
        regs = (PROC_REGS *)((uint8_t *)p->ustack_top - sizeof(PROC_REGS));
        regs->cs = DEFAULT_UMODE_CODE;
        regs->ss = DEFAULT_UMODE_DATA;

        klogd("PROCESS CREATED: New user mode process (%d):\n", p->id);
        klogd("USTACK TOP: %x | USTACK BOTTOM: %x\n", p->ustack_top, p->ustack_bottom);
        klogd("KSTACK TOP: %x | KSTACK BOTTOM: %x\n", p->kstack_top, p->kstack_bottom);
    } else {
        /* Kernel-mode process: allocate only the kernel stack */
        p->kstack_bottom = kmalloc(STACK_SIZE);
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

    p->addrspace = vaddr_space; // In kernel mode, this remains NULL.
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

    /* Safely copy the process name ensuring null termination */
    strncpy(p->name, name, sizeof(p->name) - 1);
    p->name[sizeof(p->name) - 1] = '\0';

    /* Set current working directory to "/" */
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
    memset(child, 0, sizeof(PROCESS));

    /* Manually copy only the necessary fields instead of a full memcpy */
    child->mode = parent->mode;
    child->priority = parent->priority;
    child->parent_id = parent->id;
    child->last_tick = parent->last_tick;
    child->state = parent->state;
    strncpy(child->name, parent->name, sizeof(child->name) - 1);
    child->name[sizeof(child->name) - 1] = '\0';
    strncpy(child->cwd, parent->cwd, sizeof(child->cwd) - 1);
    child->cwd[sizeof(child->cwd) - 1] = '\0';

    /* TODO: Do the vector lists need to be initialized here? */
    memset(&child->memmap_list, 0, sizeof(child->memmap_list));
    memset(&child->child_list, 0, sizeof(child->child_list));
    hash_init(&child->open_files);

    /* Create a new address space for the child process */
    child->addrspace = create_address_space();
    if (!child->addrspace) {
        kloge("Failed to create address space for forked process!\n");
        kfree(child);
        return NULL;
    }

    /* Duplicate parent's memory map entries */
    for (size_t i = 0; i < vector_len(&parent->memmap_list); i++) {
        MEM_MAP m = vector_at(&parent->memmap_list, i);
        void *temp = kmalloc(m.num_pages * PAGE_SIZE);
        if (!temp) {
            kloge("Failed to allocate memory during fork for memmap duplication!\n");
            goto fork_error_cleanup;
        }
        uint64_t ptr = VIRT_TO_PHYS(temp);
        memcpy((void *)(PHYS_TO_VIRT(ptr)), (const void *)(PHYS_TO_VIRT(m.phys_addr)),
               m.num_pages * PAGE_SIZE);
        vm_map(child->addrspace, m.virt_addr, ptr, m.num_pages, m.flags);
        m.phys_addr = ptr;
        vector_append(&child->memmap_list, m);
    }

    /* Set child process ID and update process counters */
    child->id = curr_pid;
    curr_pid++;
    num_processes++;

    /* Duplicate the kernel stack by allocating a new one and copying content */
    child->kstack_bottom = kmalloc(STACK_SIZE);
    if (!child->kstack_bottom) {
        kloge("Failed to allocate kernel stack for forked process!\n");
        goto fork_error_cleanup;
    }
    memcpy(child->kstack_bottom, parent->kstack_bottom, STACK_SIZE);
    /* Recalculate the thread stack pointer based on offset */
    uint64_t offset = (uint64_t)parent->tstack_top - (uint64_t)parent->kstack_bottom;
    child->kstack_top = (void *)((uint8_t *)child->kstack_bottom + STACK_SIZE);
    child->tstack_top = (void *)((uint8_t *)child->kstack_bottom + offset);

    /* Adjust registers on the child’s thread stack if within kernel stack range */
    if ((uint64_t)child->tstack_top >= (uint64_t)child->kstack_bottom &&
        (uint64_t)child->tstack_top <= (uint64_t)((uint8_t *)child->kstack_bottom + STACK_SIZE)) {
        PROC_REGS *regs = (PROC_REGS *)child->tstack_top;
        offset = regs->rsp - (uint64_t)parent->kstack_bottom;
        regs->rsp = (uint64_t)child->kstack_bottom + offset;
        offset = regs->rbp - (uint64_t)parent->kstack_bottom;
        regs->rbp = (uint64_t)child->kstack_bottom + offset;
    }

    /* Duplicate open file descriptors with deep copy */
    memcpy(&child->open_files, &parent->open_files, sizeof(HASH));
    for (size_t i = 0; i < child->open_files.size; i++) {
        if (child->open_files.entries[i].key == -1 ||
            child->open_files.entries[i].data == NULL) {
            continue;
        }
        VFS_NODE_DESC *desc = kmalloc(sizeof(VFS_NODE_DESC));
        if (!desc) {
            kloge("Failed to allocate memory for open file descriptor duplication!\n");
            goto fork_error_cleanup;
        }
        memcpy(desc, child->open_files.entries[i].data, sizeof(VFS_NODE_DESC));
        child->open_files.entries[i].data = desc;
        desc->inode->references++;
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
        TODO: Fix kfree error when trying to free certain parts
              of memory maps.
    */
    for (size_t i = 0; i < vector_len(&p->memmap_list); i++) {
        MEM_MAP m = vector_at(&p->memmap_list, i);
        vm_unmap(p->addrspace, m.virt_addr, m.num_pages);
        kfree((void *)PHYS_TO_VIRT(m.phys_addr));
    }
    vector_free(&p->memmap_list);
    vector_free(&p->child_list);
    vector_free(&p->dup_list);

    if (p->kstack_bottom)
        kfree(p->kstack_bottom);

    /* Free all memory in the address space memory list */
    for (size_t i = 0; i < vector_len(&p->addrspace->memory_list); i++) {
        uint64_t m = vector_at(&p->addrspace->memory_list, i);
        pm_free(m, 8);
    }
    vector_free(&p->addrspace->memory_list);

    klogd("FREEING PROCESS: Process ID: %d\n", p->id);

    kfree(p->addrspace->pml4);
    kfree(p->addrspace);
    kfree(p);
}