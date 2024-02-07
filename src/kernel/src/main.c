#include <stdint.h>
#include <boot/bootparams.h>
#include <hal/init.h>
#include "../include/memory.h"
#include "../include/stdio.h"

#define MEM_USABLE (0x1)
extern void _init();

void start(BOOT_PARAMS *params) {
    /* crti/crtn startup (handled by GCC) */
    _init();

    clear_screen();

    /* Sets vital system settings */
    system_init();

    /* Calculate available memory */
    uint64_t bytes = 0;
    for (int i = 0; i < params->memory.region_count; i++) {
        if (params->memory.regions->type == MEM_USABLE) {
            bytes += params->memory.regions->length;
        }
    }
    printf("Usable memory: 0x%llx (%d bytes)\n", bytes, bytes);

    for (;;);
}