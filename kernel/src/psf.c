#include <psf.h>

int psf1_get_glyphs(LIMINE_FILE *file, PSF1_FONT **font) {
    kprintf("Into psf1_get_glyphs\n");
    kprintf("*font: %x | font: %x\n", *font, font);
    // (*font) is NULL
    (*font)->header = (PSF1_HEADER *) file->address;
    kprintf("Header is set\n");
    if (!PSF1_MAGIC_CHECK((*font)->header->magic)) {
        return NOT_PSF1;
    }

    kprintf("header is all good!\n");

    (*font)->glyph_buffer = (void *)((uint64_t) file->address
                                      + sizeof(PSF1_HEADER));
    kprintf("Glyph buffer is set\n");
    return PSF1_SUCCESS;
}