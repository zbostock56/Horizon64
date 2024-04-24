#include <kernel.h>

void _start() {

    if (!LIMINE_BASE_REVISION_SUPPORTED) {
        halt();
    }

    if (!framebuffer_req.response ||
        framebuffer_req.response->framebuffer_count < 1) {
        halt();
    }

    /* Sets vital system settings */
    //system_init();


    struct limine_framebuffer *framebuffer =
        framebuffer_req.response->framebuffers[0];

    for (size_t i = 0; i < 500000; i++) {
        uint32_t *fb_ptr = framebuffer->address;
        fb_ptr[i] = i;
    }

    /* Calculate available memory */

    halt();
}