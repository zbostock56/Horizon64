/**
 * @file vm_map.c
 * @author Zack Bostock
 * @brief Functionality pertaining to vm_map system call
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <proc/syscalls/vm_map.h>
#include <proc/process.h>
#include <proc/syscall.h>
#include <proc/ctxsw.h>

#include <sys/smp.h>

#include <common/lock.h>

#define MMAP_ANON_BASE  (0x80000000000)

extern LOCK ctxsw_lock;

/**
 * @brief Syscall implementation for allocating memory
 *
 * @param hint Where to allocate at
 * @param len Number of bytes to allocate
 * @param prot Flags
 * @param flags More flags
 * @param fd Descriptor to allocate to (unused currently)
 * @param offset Offset (unused currently)
 * @return uint64_t
 */
uint64_t sys_vm_map(uint64_t *hint, uint64_t len, uint64_t prot,
                    uint64_t flags, uint64_t fd, uint64_t offset) {
    (void) fd;
    (void) offset;
    (void) prot;

    /* Reset errno */
    cpu_set_errno(0);

    PROCESS *pcurr = sched_get_curr_proc();
    ADDR_SPACE *as = NULL;

    if (pcurr) {
        if (pcurr->id < 1) {
            kloge("sys_vm_map: Found corrupted process id\n");
            halt();
        }
        if (!(as = pcurr->addrspace)) {
            cpu_set_errno(EINVAL);
            kloge("sys_vm_map: address space does not exist\n");
            goto err_exit;
        }
    }

    if (len == 0) {
        cpu_set_errno(EINVAL);
        goto err_exit;
    }

    if ((flags & MAP_ANONYMOUS) == 0) {
        cpu_set_errno(ENODEV);
        goto err_exit;
    }

    size_t pf = VM_USERMODE;
    uint64_t ptr = (uint64_t) hint;
    uint64_t np = NUM_PAGES(len);

    /* Unmap before mapping to newly allocated memory block */
    if (ptr != (uint64_t) NULL) {
        vm_unmap(as, ptr, np);
    }

    uint64_t phys_ptr = VIRT_TO_PHYS(kcmalloc(np * PAGE_SIZE));

    /* On real hardward, the bits might not be zeroed out */
    memset((void *)(PHYS_TO_VIRT(phys_ptr)), 0, np * PAGE_SIZE);

    if (!(flags & MAP_FIXED)) {
        ptr = phys_ptr + MMAP_ANON_BASE;
    }

    vm_map(as, ptr, phys_ptr, NUM_PAGES(len), pf);

    MEM_MAP m = {
        .virt_addr = ptr,
        .phys_addr = phys_ptr,
        .num_pages = NUM_PAGES(len),
        .flags = pf
    };

    LOCK_LOCK(&ctxsw_lock);
    vector_append(&pcurr->memmap_list, m);
    UNLOCK_LOCK(&ctxsw_lock);

    return ptr;

    err_exit:
        kloge("sys_vm_map: process %d: failed malloc\n", pcurr->id);
        return -1;
}
