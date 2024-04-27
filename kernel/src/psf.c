#include <psf.h>

int psf1_get_glyphs(LIMINE_FILE *file) {
    font.header = (PSF1_HEADER *) file->address;
    // if (!PSF1_MAGIC_CHECK(font.header)) {
    //     return NOT_PSF1;
    // }
    font.glyph_buffer = (void *) ((uint64_t) file->address + 
                        sizeof(PSF1_HEADER));

    return PSF1_SUCCESS;
}

void psf1_font_init(struct limine_module_request req, const char *path) {

    if (!req.response) {
        kprintf("psf1 file request is NULL!\n");
        halt();
    }

    LIMINE_FILE *psf_font_file;
    get_iso_file(path, req, &psf_font_file);

    if (!psf_font_file) {
        kprintf("The returned struct from get_iso_file is NULL!\n");
        halt();
    }

    if (psf1_get_glyphs(psf_font_file) != PSF1_SUCCESS) {
        kprintf("Failed to get glyphs from PSF1 font file!\n");
        halt();
    }

    kprintf("Successfully initialized %s\n", path);
}