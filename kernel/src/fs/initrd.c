/**
 * @file initrd.c
 * @author Zack Bostock
 * @brief Functionality pertaining to loading the initrd
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <fs/initrd.h>
#include <fs/ramfs.h>

#include <common/string.h>
#include <common/limine_typedefs.h>

#include <init/iso_file.h>

#include <sys/asm.h>
#include <sys/mmu.h>

extern ADDR_SPACE kernel_addr_space;

/**
 * @brief Helper for loading the Initial Ram Disk (initrd) upon system startup
 */
void initrd_init(struct limine_module_request req) {
    klogs("INIT INITRD: starting...\n");

    if (!req.response) {
        kloge("INIT INITRD: Limine module response is NULL!\n");
        halt();
    }

    LIMINE_MODULE_RESP *res = req.response;
    int found = FALSE;
    for (size_t i = 0; i < res->module_count; i++) {
        LIMINE_FILE *file = res->modules[i];
        if (check_string_ending(file->path, "initrd.tar")) {
            vm_map(&kernel_addr_space, (uint64_t)file->address,
                    VIRT_TO_PHYS(file->address), NUM_PAGES(file->size),
                    VM_DEFAULT);
            klogd("Found initrd at %x with file size %d\n", file->address, file->size);
            init_ramfs(file->address, file->size);
            found = TRUE;
        }
    }

    if (!found) {
        kloge("INIT INITRD: Cannot find INITRD module!\n");
        halt();
    }

    klogs("INIT INITRD: finished...\n");
}