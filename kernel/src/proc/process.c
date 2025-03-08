/**
 * @file process.c
 * @author Zack Bostock
 * @brief Basic operations for creating and removing processes
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <proc/process.h>

#include <common/kprint.h>
#include <common/kmalloc.h>

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
 * @param paddr_space Physical addresss space of the process
 * @return PROCESS* Newly created process
 */
PROCESS *process_create(const char *name, void (*entry)(PROC_ID), PROC_PRIO prio,
                       PROC_MODE mode, ADDR_SPACE *paddr_space) {

    /* TODO: Look into potential speed issues here. */

    if (num_processes >= max_processes) {
        kloge("Process limit hit!\n");
        return NULL;
    }

    PROCESS *p = (PROCESS *) (kmalloc(sizeof(PROCESS)));
    if (!p) {
        kloge("Failed to allocate memory for a new process!\n");
        return NULL;
    }

    memset(p, 0, sizeof(PROCESS));

    /* Set the process ID */
    p->id = curr_pid;

    /* This process is being created, so we set this to false */
    p->is_forked = FALSE;

    PROC_REGS *regs = NULL;

    /* All kernel processes share the same address space */
    ADDR_SPACE *vaddr_space = NULL;

    if (mode == PROC_UMODE) {
        vaddr_space = create_address_space();

        /* Allocate the kernel stack */
        p->kstack_bottom = (void *) (kmalloc(STACK_SIZE));
        if (!p->kstack_bottom) {
            kloge("Failed to allocate space for kernel stack!\n");
            return NULL;
        }
        p->kstack_top = p->kstack_bottom + STACK_SIZE;

        /* Allocate the user stack */
        p->ustack_bottom = (void *) (kmalloc(STACK_SIZE));
        if (!p->ustack_bottom) {
            kloge("Failed to allocate space for user stack!\n");
            return NULL;
        }
        /* Make separation for the kernel and user memory */
        p->ustack_bottom = (void *)(VIRT_TO_PHYS(p->ustack_bottom));
        p->ustack_top = p->ustack_bottom + STACK_SIZE;

        /* Set the current stack to be the user stack since we're */
        /* running in user mode */
        p->tstack_bottom = p->ustack_bottom;
        p->tstack_top = p->ustack_top;

        vm_map(paddr_space, (uint64_t) p->ustack_bottom,
               (uint64_t) p->ustack_bottom,
                NUM_PAGES(STACK_SIZE), VM_DEFAULT | VM_USERMODE);

        vm_map(vaddr_space, (uint64_t) p->ustack_bottom,
               (uint64_t) p->ustack_bottom,
                NUM_PAGES(STACK_SIZE), VM_DEFAULT | VM_USERMODE);

        MEM_MAP m = {
            .virt_addr = (uint64_t) p->ustack_bottom,
            .phys_addr = (uint64_t) p->ustack_bottom,
            .num_pages = NUM_PAGES(STACK_SIZE),
            .flags = VM_DEFAULT | VM_USERMODE
        };

        vector_append(&p->memmap_list, m);

        regs = p->ustack_top - sizeof(PROC_REGS);

        regs->cs = DEFAULT_UMODE_CODE;
        regs->ss = DEFAULT_UMODE_DATA;

        klogd("PROCESS CREATED: New user mode process (%d):\n", p->id);
        klogd("USTACK TOP: %x | USTACK BOTTOM: %x\n",
              p->ustack_top, p->ustack_bottom);
        klogd("KSTACK TOP: %x | KSTACK BOTTOM: %x\n",
              p->kstack_top, p->kstack_bottom);
    } else {
        p->kstack_bottom = kmalloc(STACK_SIZE);
        if (!p->kstack_bottom) {
            kloge("Failed to allocate space for kernel stack!\n");
            return NULL;
        }
        p->kstack_top = p->kstack_bottom + STACK_SIZE;

        /* Process runs in kernel mode, no need for user stack */
        p->ustack_bottom = NULL;
        p->ustack_top = NULL;

        klogd("PROCESS CREATED: New kernel mode process (%d):\n",
               p->id);
        klogd("STACK TOP: %x | STACK BOTTOM: %x\n",
               p->id, p->kstack_top, p->kstack_bottom);

        /* Set the currently used stack */
        p->tstack_bottom = p->kstack_bottom;
        p->tstack_top = p->kstack_top;

        regs = p->kstack_top - sizeof(PROC_REGS);
        regs->cs = DEFAULT_KMODE_CODE;
        regs->ss = DEFAULT_KMODE_DATA;
    }

    p->addrspace = vaddr_space;

    regs->rsp = (uint64_t) p->tstack_top;
    regs->rflags = DEFAULT_RFLAGS;
    regs->rip = (uint64_t) entry;
    regs->rdi = curr_pid;

    p->mode = mode;
    p->tstack_top = regs;
    p->parent_id = UINT64_MAX;
    p->priority = prio;
    p->last_tick = 0;
    p->state = PROC_READY;

    strncpy(p->cwd, "/", sizeof("/"));
    strncpy(p->name, name, strlen(name));

    hash_init(&p->open_files);

    curr_pid++;
    num_processes++;

    if (mode == PROC_UMODE) {
        vm_unmap(paddr_space, (uint64_t) p->ustack_bottom, NUM_PAGES(STACK_SIZE));
    }

    return p;
}

/**
 * @brief Process forking function
 *
 * @param parent Parent to the new child process
 * @return PROCESS* Newly forked child process
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

    PROCESS *child = (PROCESS *) (kmalloc(sizeof(PROCESS)));
    if (!child) {
        kloge("Cannot allocate memory for forked process!\n");
        return NULL;
    }

    /* Take the parent process as a template for the forked process */
    memcpy(child, parent, sizeof(PROCESS));

    memset(&child->memmap_list, 0, sizeof(child->memmap_list));
    memset(&child->child_list, 0, sizeof(child->child_list));

    child->is_forked = TRUE;
    child->addrspace = create_address_space();

    /* Copy memory over to forked process */
    for (size_t i = 0; i < vector_len(&(parent->memmap_list)); i++) {
        MEM_MAP m = vector_at(&(parent->memmap_list), i);
        uint64_t ptr = VIRT_TO_PHYS(kmalloc(m.num_pages * PAGE_SIZE));
        memcpy((void *) PHYS_TO_VIRT(ptr), (void *) PHYS_TO_VIRT(m.phys_addr),
                m.num_pages * PAGE_SIZE);
        vm_map(child->addrspace, m.virt_addr, ptr, m.num_pages, m.flags);
        m.phys_addr = ptr;
        vector_append(&child->memmap_list, m);
    }

    child->id = curr_pid;
    child->parent_id = parent->id;

    child->kstack_bottom = kmalloc(STACK_SIZE);
    memcpy(child->kstack_bottom, parent->kstack_bottom, STACK_SIZE);

    uint64_t offset = (uint64_t) child->kstack_top - (uint64_t) parent->kstack_bottom;
    child->kstack_top = (void *) ((uint64_t) child->kstack_bottom + offset);

    if ((uint64_t) child->tstack_top >= (uint64_t) parent->kstack_bottom &&
        (uint64_t) child->tstack_top <= (uint64_t) (parent->kstack_bottom + STACK_SIZE)) {
        offset = (uint64_t) child->tstack_top - (uint64_t) parent->tstack_bottom;
        child->tstack_top = (void *)((uint64_t) child->kstack_bottom + offset);

        PROC_REGS *regs = (PROC_REGS *) child->tstack_top;

        offset = (uint64_t) regs->rsp - (uint64_t) parent->kstack_bottom;
        regs->rsp = (uint64_t) child->kstack_bottom + offset;

        offset = (uint64_t) regs->rbp - (uint64_t) parent->kstack_bottom;
        regs->rbp = (uint64_t) child->kstack_bottom + offset;
    }

    /* Increase the reference count of all open file descriptors */
    memcpy(&child->open_files, &parent->open_files, sizeof(HASH));
    for (size_t i = 0; i < child->open_files.size; i++) {
        if (child->open_files.entries[i].key == -1 ||
            child->open_files.entries[i].data == NULL) {
            continue;
        }

        VFS_NODE_DESC *desc = (VFS_NODE_DESC *) (kmalloc(sizeof(VFS_NODE_DESC)));
        memcpy(desc, child->open_files.entries[i].data, sizeof(VFS_NODE_DESC));
        child->open_files.entries[i].data = desc;
        desc->inode->references++;
    }

    vector_append(&parent->child_list, child->id);
    curr_pid++;
    num_processes++;

    klogd("PROCESS FORKED: Parent: %d | New ID: %d\n", child->parent_id, child->id);

    return child;
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

    /* Free all parts of the process's memory */
    for (size_t i = 0; i < vector_len(&p->memmap_list); i++) {
        MEM_MAP m = vector_at(&p->memmap_list, i);
        vm_unmap(p->addrspace, m.virt_addr, m.num_pages);
        kfree((void *) PHYS_TO_VIRT(m.phys_addr));
    }

    vector_free(&p->memmap_list);
    vector_free(&p->child_list);
    vector_free(&p->dup_list);

    kfree((void *) p->kstack_bottom);

    for (size_t i = 0; i < vector_len(&p->addrspace->memory_list); i++) {
        uint64_t m = vector_at(&p->addrspace->memory_list, i);
        pm_free(m, 8);
    }

    vector_free(&p->addrspace->memory_list);

    klogd("FREEING PROCESS: Process ID: %d\n", p->id);

    kfree((void *) p->addrspace->pml4);
    kfree((void *) p->addrspace);
    kfree(p);
}