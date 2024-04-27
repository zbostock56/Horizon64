#include <init.h>

/*
    Main function of the "HAL" (hardware abstraction layer).

    Sets up GDT, IDT, ISR, IRQ, and other important system
    variables.
*/
#include <psf.h>

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

    kprintf("halting...");
    halt();

    /* Initial framebuffer */
    // if (!framebuffer_req.response ||
    //     framebuffer_req.response->framebuffer_count < 1) {
    //     kprintf("Framebuffer either no response or not correct count!\n");
    //     halt();
    // }

    // struct limine_framebuffer *fb = framebuffer_req.response->framebuffers[0];

    // initial_fb.base = fb->address;
    // initial_fb.height = fb->height;
    // initial_fb.width = fb->width;
    // initial_fb.pitch = fb->pitch;
    // initial_fb.bpp = fb->bpp;
    // initial_fb.cursor_pos = IVEC2_ZERO;

    // kprintf("Initialized framebuffer:\n");
    // kprintf("initial_fb->base: %x\n", initial_fb.base);
    // kprintf("initial_fb->height: %d\n", initial_fb.height);
    // kprintf("initial_fb->width: %d\n", initial_fb.width);
    // kprintf("initial_fb->pitch: %d\n", initial_fb.pitch);
    // kprintf("initial_fb->bpp: %d\n", initial_fb.bpp);

    // /* Set up .psf font */
    // if (!psf_file_request.response) {
    //     kprintf("psf file request is NULL!\n");
    //     halt();
    // }
    // LIMINE_FILE *psf_font_file;
    // get_iso_file("zap-vga16.psf", psf_file_request, &psf_font_file);
    // kprintf("Made it through getting iso file\n");

    // if (!psf_font_file) {
    //     kprintf("The returned struct from get_iso_file is NULL!\n");
    //     halt();
    // }

    // kprintf("Going into psf1_get_glphys\n");
    // if (psf1_get_glyphs(psf_font_file, (PSF1_FONT **) &(initial_fb.font)) != PSF1_SUCCESS) {
    //     kprintf("An error occured in getting the glyphs from the .psf file!\n");
    //     halt();
    // }

    // kprintf("Finished\n");
    // halt();

    // PSF1_FONT *f = (PSF1_FONT *) initial_fb.font;
    // f->header = (PSF1_HEADER *) psf_font_file->address;
    // kprintf("Header is set\n");
    // if (!PSF1_MAGIC_CHECK(f->header->magic)) {
    //     kprintf("Incorrect font type!\n");
    //     return;
    // }
    // f->glyph_buffer = (void *)((uint64_t) psf_font_file->address
    //                                   + sizeof(PSF1_HEADER));
    // // initial_fb.font_type = PSF1;

    // kprintf("Made it past setting font\n");

    /* Set up hardware functionality */
    // TODO: IDT, PIT, PIC
}