#include <stdint.h>
#include "../include/stdio.h"
#include "../include/x86.h"
#include "../include/disk.h"
#include "../include/fat.h"
#include "../include/memdefs.h"
#include "../include/memory.h"
#include "../include/elf.h"
#include "../include/memdetect.h"
#include <boot/bootparams.h>

uint8_t *kernel_load_buffer = (uint8_t *) MEMORY_LOAD_KERNEL;
uint8_t *kernel = (uint8_t *) MEMORY_KERNEL_ADDR;
BOOT_PARAMS g_params;

typedef void (* kernel_start)(BOOT_PARAMS *);

void __attribute__((cdecl)) start(uint16_t boot_drive, void *partition) {

    clear_screen();

    printf("---------- Sunrise Bootloader ----------\r\n");
    printf("Booting from drive %d\r\n", boot_drive);

    DISK disk;
    PARTITION part;

    if (!disk_initialize(&disk, boot_drive)) {
        printf("Disk initialization error!");
        goto infinite_loop;
    }

    MBR_detect_partition(&part, &disk, partition);
    if (!FAT_init(&part)) {
        printf("FAT initialization error!");
        goto infinite_loop;
    }

    /* Pass boot parameters to kernel */
    g_params.boot_device = boot_drive;
    memory_detect(&g_params.memory);

    /* Load kernel into memory */
    kernel_start kernel_entry;
    if (!ELF_read(&part, "/boot/kernel.elf", (void **) &kernel_entry)) {
        printf("ELF read failed, halting...\n");
        goto infinite_loop;
    }

    /* Hand over control to the kernel */
    kernel_entry(&g_params);

    /* Should never reach here */
    infinite_loop:
        for (;;);
}
