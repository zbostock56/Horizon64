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

/* User virtual memory layout */
#define USER_STACK_VBASE    0x7FFFF0000000UL  /* User stack virtual base */

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
 * @brief Helper to setup kernel stack for a process
 *
 * @param p Process to setup kernel stack for
 * @return STATUS SYS_OK on success, SYS_ERR on failure
 */
static STATUS process_setup_kernel_stack(PROCESS *p) {
    if (!p) return SYS_ERR;

    p->kstack_bottom = kcmalloc(STACK_SIZE);
    if (!p->kstack_bottom) {
        kloge("Failed to allocate kernel stack!\n");
        return SYS_ERR;
    }

    p->kstack_top = (void *)((uint8_t *)p->kstack_bottom + STACK_SIZE);
    klogd("Allocated kernel stack: bottom=%x, top=%x\n", p->kstack_bottom, p->kstack_top);
    return SYS_OK;
}

/**
 * @brief Helper to setup user stack for a process
 *
 * @param p Process to setup user stack for
 * @param paddr_space Parent address space for mapping
 * @param vaddr_space Virtual address space for mapping
 * @return STATUS SYS_OK on success, SYS_ERR on failure
 */
static STATUS process_setup_user_stack(PROCESS *p, ADDR_SPACE *paddr_space,
                                       ADDR_SPACE *vaddr_space) {
    // if (!p || !paddr_space || !vaddr_space) return SYS_ERR;
    (void) paddr_space;

    /* Allocate physical memory for user stack */
    uint64_t phys_ustack = pm_get(NUM_PAGES(STACK_SIZE), 0x0, __func__, __LINE__);
    if (!phys_ustack) {
        kloge("Failed to allocate physical memory for user stack!\n");
        return SYS_ERR;
    }

    /* Calculate virtual address for user stack */
    uint64_t virt_ustack = USER_STACK_VBASE;

    /* Set up stack pointers with VIRTUAL addresses */
    p->ustack_bottom = (void *)virt_ustack;
    p->ustack_top = (void *)(virt_ustack + STACK_SIZE);

    /* Use user stack as thread stack */
    p->tstack_bottom = p->ustack_bottom;
    p->tstack_top = p->ustack_top;

    /* Map the user stack in the virtual address space only */
    if (vm_map_impl(vaddr_space, virt_ustack, phys_ustack,
                    NUM_PAGES(STACK_SIZE), VM_DEFAULT | VM_USERMODE) != MMU_SUCCESS) {
        kloge("Failed to map user stack in virtual address space!\n");
        pm_free(phys_ustack, NUM_PAGES(STACK_SIZE));
        return SYS_ERR;
    }

    /* Add to memory map list with correct virtual and physical addresses */
    MEM_MAP m = {
        .virt_addr = virt_ustack,
        .phys_addr = phys_ustack,
        .num_pages = NUM_PAGES(STACK_SIZE),
        .flags = VM_DEFAULT | VM_USERMODE
    };

    if (vector_append(&p->memmap_list, m) != 0) {
        kloge("Failed to add user stack to memory map list!\n");
        vm_unmap(vaddr_space, virt_ustack, NUM_PAGES(STACK_SIZE));
        pm_free(phys_ustack, NUM_PAGES(STACK_SIZE));
        return SYS_ERR;
    }

    klogd("Allocated user stack: virt=%x-%x, phys=%x\n",
          p->ustack_bottom, p->ustack_top, (void*)phys_ustack);
    return SYS_OK;
}

/**
 * @brief Helper to initialize process registers
 *
 * @param p Process to initialize registers for
 * @param entry Entry point of the process
 * @param mode Process mode (PROC_UMODE or PROC_KMODE)
 * @return STATUS SYS_OK on success, SYS_ERR on failure
 */
static STATUS process_init_registers(PROCESS *p, void (*entry)(PROC_ID), PROC_MODE mode) {
    // if (!p || !entry) return SYS_ERR;

    PROC_REGS *regs;

    if (mode == PROC_UMODE) {
        /* For user mode processes, we need to access the user stack through
         * the physical address since it's not mapped in kernel space */
        uint64_t stack_virt = (uint64_t)p->tstack_top - sizeof(PROC_REGS);

        /* Verify we have enough stack space */
        if (stack_virt < (uint64_t)p->tstack_bottom) {
            kloge("Not enough stack space for registers!\n");
            return SYS_ERR;
        }

        /* Find the physical address corresponding to this virtual address */
        uint64_t phys_addr = 0;
        for (size_t i = 0; i < vector_len(&p->memmap_list); i++) {
            MEM_MAP m = vector_at(&p->memmap_list, i);
            if (stack_virt >= m.virt_addr &&
                stack_virt < m.virt_addr + (m.num_pages * PAGE_SIZE)) {
                /* Found the mapping, calculate physical address */
                uint64_t offset = stack_virt - m.virt_addr;
                phys_addr = m.phys_addr + offset;
                break;
            }
        }

        if (!phys_addr) {
            kloge("Could not find physical address for user stack registers!\n");
            return SYS_ERR;
        }

        /* Access registers through kernel's higher-half mapping */
        regs = (PROC_REGS *)PHYS_TO_VIRT(phys_addr);

        /* Update thread stack top to point to virtual address */
        p->tstack_top = (void *)stack_virt;

    } else {
        /* For kernel mode processes, registers are on kernel stack */
        regs = (PROC_REGS *)((uint8_t *)p->tstack_top - sizeof(PROC_REGS));

        /* Verify we have enough stack space */
        if ((void *)regs < p->tstack_bottom) {
            kloge("Not enough stack space for registers!\n");
            return SYS_ERR;
        }

        /* Update thread stack top to point to registers */
        p->tstack_top = regs;
    }

    /* Initialize registers based on mode */
    memset(regs, 0, sizeof(PROC_REGS));

    if (mode == PROC_UMODE) {
        regs->cs = DEFAULT_UMODE_CODE;
        regs->ss = DEFAULT_UMODE_DATA;
    } else {
        regs->cs = DEFAULT_KMODE_CODE;
        regs->ss = DEFAULT_KMODE_DATA;
    }

    regs->rsp = (uint64_t)p->tstack_top;
    regs->rflags = DEFAULT_RFLAGS;
    regs->rip = (uint64_t)entry;
    regs->rdi = p->id;  /* Pass process ID as first argument */

    klogd("Initialized registers for process %d at virt=%x, phys=%x\n",
          p->id, p->tstack_top, (void*)((mode == PROC_UMODE) ? VIRT_TO_PHYS(regs) : (uint64_t)regs));
    return SYS_OK;
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

    if (num_processes >= max_processes) {
        kloge("Process limit hit! Current: %d, Max: %d\n", num_processes, max_processes);
        return NULL;
    }

    /* Pre-increment PID to avoid reuse on failure */
    PROC_ID new_pid = curr_pid++;

    /* Allocate process structure */
    PROCESS *p = (PROCESS *)kmalloc(sizeof(PROCESS));
    if (!p) {
        kloge("Failed to allocate memory for new process!\n");
        curr_pid--; /* Rollback PID on failure */
        return NULL;
    }

    /* Initialize process structure */
    memset(p, 0, sizeof(PROCESS));
    p->id = new_pid;
    p->is_forked = FALSE;
    p->mode = mode;
    p->priority = prio;
    p->last_tick = 0;
    p->state = PROC_READY;
    p->parent_id = UINT64_MAX;

    /* Initialize collections */
    if (vector_init(&p->memmap_list) != 0 ||
        vector_init(&p->child_list) != 0 ||
        vector_init(&p->dup_list) != 0) {
        kloge("Failed to initialize process vectors!\n");
        kfree(p);
        curr_pid--; /* Rollback PID on failure */
        return NULL;
    }

    hash_init(&p->open_files);

    /* Copy name and set default working directory */
    strncpy(p->name, name, sizeof(p->name) - 1);
    p->name[sizeof(p->name) - 1] = '\0';
    strncpy(p->cwd, "/", sizeof(p->cwd) - 1);
    p->cwd[sizeof(p->cwd) - 1] = '\0';

    /* Setup kernel stack (required for all processes) */
    if (process_setup_kernel_stack(p) != SYS_OK) {
        goto error_cleanup;
    }

    ADDR_SPACE *vaddr_space = NULL;

    if (mode == PROC_UMODE) {
        /* Create virtual address space for user mode process */
        vaddr_space = create_address_space();
        if (!vaddr_space) {
            kloge("Failed to create address space for user mode process!\n");
            goto error_cleanup;
        }

        /* Setup user stack */
        if (process_setup_user_stack(p, paddr_space, vaddr_space) != SYS_OK) {
            goto error_cleanup;
        }

        klogd("PROCESS CREATED: New user mode process (%d):\n", p->id);
        klogd("USTACK: %x-%x | KSTACK: %x-%x\n",
              p->ustack_bottom, p->ustack_top, p->kstack_bottom, p->kstack_top);
    } else {
        /* Kernel mode: use kernel stack as thread stack */
        p->ustack_bottom = NULL;
        p->ustack_top = NULL;
        p->tstack_bottom = p->kstack_bottom;
        p->tstack_top = p->kstack_top;

        klogd("PROCESS CREATED: New kernel mode process (%d):\n", p->id);
        klogd("KSTACK: %x-%x\n", p->kstack_bottom, p->kstack_top);
    }

    /* Set address space */
    p->addrspace = vaddr_space;

    /* Initialize registers */
    if (process_init_registers(p, entry, mode) != SYS_OK) {
        goto error_cleanup;
    }

    /* Increment process counter only on success */
    num_processes++;

    klogd("Successfully created process '%s' with ID %d\n", p->name, p->id);
    return p;

error_cleanup:
    kloge("Error during process creation, cleaning up...\n");

    if (p) {
        /* Free vectors */
        vector_free(&p->memmap_list);
        vector_free(&p->child_list);
        vector_free(&p->dup_list);

        /* Free stacks */
        if (p->kstack_bottom) {
            kcfree(p->kstack_bottom);
        }

        /* For user mode processes, clean up user stack */
        if (mode == PROC_UMODE) {
            /* Free memory mappings */
            for (size_t i = 0; i < vector_len(&p->memmap_list); i++) {
                MEM_MAP m = vector_at(&p->memmap_list, i);
                if (vaddr_space) {
                    vm_unmap(vaddr_space, m.virt_addr, m.num_pages);
                }
                pm_free(m.phys_addr, m.num_pages);
            }
        }

        /* Free address space */
        if (vaddr_space) {
            /* Clean up address space memory list */
            for (size_t i = 0; i < vector_len(&vaddr_space->memory_list); i++) {
                uint64_t m = vector_at(&vaddr_space->memory_list, i);
                pm_free(m, DEFAULT_PAGES);
            }
            vector_free(&vaddr_space->memory_list);
            if (vaddr_space->pml4) {
                kcfree(vaddr_space->pml4);
            }
            kfree(vaddr_space);
        }

        kfree(p);
    }

    /* Rollback PID counter */
    curr_pid--;

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

        /* Allocate new physical memory for the child */
        uint64_t new_phys_addr = pm_get(m.num_pages, 0x0, __func__, __LINE__);
        if (!new_phys_addr) {
            kloge("Failed to allocate memory during memmap duplication!\n");
            return SYS_ERR;
        }

        /* Copy memory content */
        void *src = (void *)PHYS_TO_VIRT(m.phys_addr);
        void *dst = (void *)PHYS_TO_VIRT(new_phys_addr);
        memcpy(dst, src, m.num_pages * PAGE_SIZE);

        /* Map in child's address space */
        if (vm_map_impl(child->addrspace, m.virt_addr, new_phys_addr,
                        m.num_pages, m.flags) != MMU_SUCCESS) {
            kloge("Failed to map duplicated memory in child address space!\n");
            pm_free(new_phys_addr, m.num_pages);
            return SYS_ERR;
        }

        /* Update memory map entry and add to child's list */
        m.phys_addr = new_phys_addr;
        if (vector_append(&child->memmap_list, m) != 0) {
            kloge("Failed to add memory map to child process!\n");
            vm_unmap(child->addrspace, m.virt_addr, m.num_pages);
            pm_free(new_phys_addr, m.num_pages);
            return SYS_ERR;
        }
    }

    return SYS_OK;
}

/**
 * @brief Helper for duplicating descriptors between two processes
 * 
 * @param parent Process to copy descriptors from
 * @param child Process to copy descriptors to
 * @param func Function name for error reporting
 * @return STATUS SYS_ERR if error, SYS_OK otherwise
 */
STATUS process_dup_file_descriptors(PROCESS *parent, PROCESS *child,
                                    const char *func) {
    if (!parent || !child || !func) {
        kloge("Invalid parameters in process_dup_file_descriptors!\n");
        halt();
    }

    for (size_t i = 0; i < parent->open_files.size; i++) {
        if (parent->open_files.entries[i].key != HASH_EMPTY_KEY &&
            parent->open_files.entries[i].data) {

            VFS_NODE_DESC *nd = (VFS_NODE_DESC *)kmalloc(sizeof(VFS_NODE_DESC));
            if (!nd) {
                kloge("%s: Failed to allocate memory for new VFS_NODE_DESC!\n", func);
                return SYS_ERR;
            }

            memcpy(nd, parent->open_files.entries[i].data, sizeof(VFS_NODE_DESC));
            child->open_files.entries[i].key = parent->open_files.entries[i].key;
            child->open_files.entries[i].data = nd;

            /* Increment reference count */
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
    if (!parent) {
        kloge("Cannot fork NULL parent process!\n");
        return NULL;
    }

    if (num_processes >= max_processes) {
        kloge("Process limit hit!\n");
        return NULL;
    }

    if (parent->mode == PROC_KMODE) {
        kloge("Cannot fork kernel process!\n");
        halt();
    }

    /* Pre-increment PID to avoid reuse on failure */
    PROC_ID new_pid = curr_pid++;

    /* Allocate child process structure */
    PROCESS *child = kmalloc(sizeof(PROCESS));
    if (!child) {
        kloge("Cannot allocate memory for forked process!\n");
        curr_pid--; /* Rollback PID on failure */
        return NULL;
    }

    /* Copy parent process structure */
    memcpy(child, parent, sizeof(PROCESS));

    /* Update child-specific fields */
    child->is_forked = TRUE;
    child->parent_id = parent->id;
    child->id = new_pid;

    /* Create new address space */
    child->addrspace = create_address_space();
    if (!child->addrspace) {
        kloge("Failed to create address space for forked process!\n");
        goto fork_error_cleanup;
    }

    /* Initialize child's collections */
    memset(&child->memmap_list, 0, sizeof(child->memmap_list));
    memset(&child->child_list, 0, sizeof(child->child_list));
    memset(&child->dup_list, 0, sizeof(child->dup_list));
    hash_init(&child->open_files);

    if (vector_init(&child->memmap_list) != 0 ||
        vector_init(&child->child_list) != 0 ||
        vector_init(&child->dup_list) != 0) {
        kloge("Failed to initialize child process vectors!\n");
        goto fork_error_cleanup;
    }

    /* Duplicate memory mappings */
    if (process_dup_memmap(parent, child) == SYS_ERR) {
        goto fork_error_cleanup;
    } 

    /* Duplicate file descriptors */
    if (process_dup_file_descriptors(parent, child, __func__) == SYS_ERR) {
        goto fork_error_cleanup;
    }

    /* Duplicate kernel stack */
    child->kstack_bottom = kcmalloc(STACK_SIZE);
    if (!child->kstack_bottom) {
        kloge("Failed to allocate kernel stack for forked process!\n");
        goto fork_error_cleanup;
    }
    memcpy(child->kstack_bottom, parent->kstack_bottom, STACK_SIZE);

    /* Calculate new kernel stack top */
    size_t kstack_offset = (uint8_t *)parent->kstack_top - (uint8_t *)parent->kstack_bottom;
    child->kstack_top = (void *)((uint8_t *)child->kstack_bottom + kstack_offset);

    /* Adjust thread stack pointers if they point to kernel stack */
    if ((uint64_t)parent->tstack_top >= (uint64_t)parent->kstack_bottom &&
        (uint64_t)parent->tstack_top < (uint64_t)parent->kstack_top) {

        size_t tstack_offset = (uint8_t *)parent->tstack_top - (uint8_t *)parent->kstack_bottom;
        child->tstack_top = (void *)((uint8_t *)child->kstack_bottom + tstack_offset);

        /* Validate register pointers before adjusting them */
        if ((uintptr_t)child->tstack_top >= (uintptr_t)child->kstack_bottom &&
            (uintptr_t)child->tstack_top + sizeof(PROC_REGS) <= (uintptr_t)child->kstack_top) {

            /* Adjust register pointers in the copied stack */
            PROC_REGS *child_regs = (PROC_REGS *)(child->tstack_top);
            PROC_REGS *parent_regs = (PROC_REGS *)(parent->tstack_top);

            /* Adjust RSP if it points within the kernel stack */
            if (parent_regs->rsp >= (uint64_t)parent->kstack_bottom &&
                parent_regs->rsp < (uint64_t)parent->kstack_top) {
                size_t rsp_offset = parent_regs->rsp - (uint64_t)parent->kstack_bottom;
                child_regs->rsp = (uint64_t)child->kstack_bottom + rsp_offset;
            }

            /* Adjust RBP if it points within the kernel stack */
            if (parent_regs->rbp >= (uint64_t)parent->kstack_bottom &&
                parent_regs->rbp < (uint64_t)parent->kstack_top) {
                size_t rbp_offset = parent_regs->rbp - (uint64_t)parent->kstack_bottom;
                child_regs->rbp = (uint64_t)child->kstack_bottom + rbp_offset;
            }
        }
    }

    /* Add child to parent's child list */
    if (vector_append(&parent->child_list, child->id) != 0) {
        kloge("Failed to add child to parent's child list!\n");
        goto fork_error_cleanup;
    }

    /* Increment process counter only on success */
    num_processes++;

    klogd("PROCESS FORKED: Parent: %d | Child: %d\n", child->parent_id, child->id);
    return child;

fork_error_cleanup:
    kloge("Fork failed, cleaning up child process...\n");

    if (child) {
        /* Free duplicated memory mappings */
        for (size_t i = 0; i < vector_len(&child->memmap_list); i++) {
            MEM_MAP m = vector_at(&child->memmap_list, i);
            if (child->addrspace) {
                vm_unmap(child->addrspace, m.virt_addr, m.num_pages);
            }
            pm_free(m.phys_addr, m.num_pages);
        }

        /* Free vectors */
        vector_free(&child->memmap_list);
        vector_free(&child->child_list);
        vector_free(&child->dup_list);

        /* Free kernel stack */
        if (child->kstack_bottom) {
            kcfree(child->kstack_bottom);
        }

        /* Free address space */
        if (child->addrspace) {
            for (size_t i = 0; i < vector_len(&child->addrspace->memory_list); i++) {
                uint64_t m = vector_at(&child->addrspace->memory_list, i);
                pm_free(m, DEFAULT_PAGES);
            }
            vector_free(&child->addrspace->memory_list);
            if (child->addrspace->pml4) {
                kcfree(child->addrspace->pml4);
            }
            kfree(child->addrspace);
        }

        kfree(child);
    }

    /* Rollback PID counter */
    curr_pid--;

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

    klogd("process_free: freeing process '%s' (pid %d)\n", p->name, p->id);

    /* Decrement process count */
    num_processes--;

    /* Free file descriptors and close open files */
    for (size_t i = 0; i < p->open_files.size; i++) {
        if (p->open_files.entries[i].key != HASH_EMPTY_KEY &&
            p->open_files.entries[i].data) {
            VFS_NODE_DESC *nd = (VFS_NODE_DESC *)p->open_files.entries[i].data;
            if (nd && nd->inode) {
                nd->inode->references--;
            }
            kfree(nd);
        }
    }

    /* Free memory mappings */
    for (size_t i = 0; i < vector_len(&p->memmap_list); i++) {
        MEM_MAP m = vector_at(&p->memmap_list, i);
        klogd("Freeing memory map %zu: virt=%x, phys=%x, pages=%d\n",
              i, (void*)m.virt_addr, (void*)m.phys_addr, m.num_pages);

        /* Unmap from virtual address space */
        if (p->addrspace) {
            vm_unmap(p->addrspace, m.virt_addr, m.num_pages);
        }

        /* Free physical memory */
        pm_free(m.phys_addr, m.num_pages);
    }
    
    /* Free vectors */
    vector_free(&p->memmap_list);
    vector_free(&p->child_list);
    vector_free(&p->dup_list);

    /* Free kernel stack */
    if (p->kstack_bottom) {
        kcfree(p->kstack_bottom);
    }

    /* Free address space */
    if (p->addrspace) {
        /* Free all memory in the address space memory list */
        for (size_t i = 0; i < vector_len(&p->addrspace->memory_list); i++) {
            uint64_t m = vector_at(&p->addrspace->memory_list, i);
            pm_free(m, DEFAULT_PAGES);
        }
        vector_free(&p->addrspace->memory_list);

        if (p->addrspace->pml4) {
            kcfree(p->addrspace->pml4);
        }
        kfree(p->addrspace);
    }

    /* Finally free the process structure */
    kfree(p);

    klogd("process_free: successfully freed process\n");
}

/**
 * @brief Helper to change the name of a process 
 * 
 * @param p Process whose name to change
 * @param name Name to change to
 */
void process_change_name(PROCESS *p, const char *name) {
    if (!p || !name) {
        kloge("Invalid parameters for process_change_name!\n");
        return;
    }

    klogd("Changing name of process %d from '%s' to '%s'\n", p->id, p->name, name);

    size_t name_len = strlen(name);
    size_t max_copy = MIN(sizeof(p->name) - 1, name_len);

    strncpy(p->name, name, max_copy);
    p->name[max_copy] = '\0';
}