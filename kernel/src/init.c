#include <init.h>

/*
    Main function of the "HAL" (hardware abstraction layer).

    Sets up GDT, IDT, ISR, IRQ, and other important system
    variables.
*/

void system_init() {
    /* Initial Limine check */
    if (!LIMINE_BASE_REVISION_SUPPORTED) {
        kprintf("Base revision not supported!\n");
        halt();
    }

    /* Initialize global descriptor table */
    gdt_init();
    idt_init();
    isr_init();

    /* Initialize framebuffer */
    fb_init(framebuffer_req);

    /* Set up .psf1 font */
    psf1_font_init(psf_file_request, "zap-vga16.psf");
    
    fb_putch(&initial_fb, 4, 0, 0xAA0000, 0x000000, 'Z');

    halt();

    // TODO: PIT, PIC
}