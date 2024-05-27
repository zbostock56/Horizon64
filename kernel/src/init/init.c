#include <init/init.h>

/*
    Sets up GDT, IDT, ISR, IRQ, and other important system
    variables.
*/

void system_init() {
    /* Initial Limine check */
    if (!LIMINE_BASE_REVISION_SUPPORTED) {
        kloge("Base revision not supported!\n");
        halt();
    }

    if (hhdm_request.response) {
      klogi("HHDM offset %x, revision %d\n",
      hhdm_request.response->offset, hhdm_request.response->revision);
    }

    /* Initialize global descriptor table */
    gdt_init();

    /* Initialize interrupt descriptor table */
    idt_init();

    /* Initialize interrupt service routines */
    isr_init();

    /* Memory initialization */
    pm_init(mem_req);

    /* Initialize framebuffer */
    fb_init(framebuffer_req);

    /* Set up .psf1 font */
    psf1_font_init(psf_file_request, "zap-vga16.psf");

    /* Initialize terminal */
    init_terminal(initial_fb);

    /* Initialize PIC, PIT */
    irq_init();

    keyboard_init();
}
