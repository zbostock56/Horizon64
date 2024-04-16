#include <stdint.h>
#include <limine.h>
#include "../hal/init.h"
#include "../include/memory.h"

//extern void _init();
static volatile LIMINE_BASE_REVISION(1);
static volatile struct limine_framebuffer_request framebuffer_req = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

void _start() {
    /* crti/crtn startup (handled by GCC) */
    //_init();

    if (!LIMINE_BASE_REVISION_SUPPORTED) {
        for (;;);
    }

    if (!framebuffer_req.response ||
        framebuffer_req.response->framebuffer_count < 1) {
        for (;;);
    }

    /* Sets vital system settings */
    //system_init();


    struct limine_framebuffer *framebuffer =
        framebuffer_req.response->framebuffers[0];

    for (size_t i = 0; i < 100; i++) {
        uint32_t *fb_ptr = framebuffer->address;
        fb_ptr[i * (framebuffer->pitch / 4) + i] = 0xffffff;
    }

    /* Calculate available memory */

    for (;;);
}