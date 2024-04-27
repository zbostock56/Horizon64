#include <kernel.h>

void _start() {

    kprintf("Into kernel\n");

    /* Sets vital system settings */
    system_init();

    /* Calculate available memory */

    // for (size_t i = 0; i < 100; i++) {
    //     volatile uint32_t *fb_ptr = initial_fb.base;
    //     fb_ptr[i * (initial_fb.pitch / 4) + i] = 0xffffff;
    // }

    halt();
}