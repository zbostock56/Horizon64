/**
 * @file psf.c
 * @author Zack Bostock
 * @brief Helpers for .psf files
 * @verbatim
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <init/psf.h>

/**
 * @brief Helper function for allocating memory for the font based on the
 *        font mode found in the PSF1_HEADER. Check header for more info.
 *
 * @return uint8_t SYS_OK if success, SYS_ERR if failure.
 */
static uint8_t psf1_font_mode() {
    size_t num_glyphs = (font.header->font_mode & PSF1_MODE512) ? 512 : 256;
    size_t buffer_size = num_glyphs * font.header->character_size;

    font.glyph_buffer = (void *)(kmalloc(buffer_size));
    if (!font.glyph_buffer) {
        kloge("INIT PSF: PSF1 glyph buffer alloc is NULL!\n");
        return SYS_ERR;
    }

    return SYS_OK;
}

/**
 * @brief Gets the glyphs from a .psf type file
 *
 * @param file File of glyphs
 * @return int Success or failure status
 */
int psf1_get_glyphs(LIMINE_FILE *file) {
    PSF1_HEADER *h = (PSF1_HEADER *) file->address;

    /* Ensure font.header is allocated before use */
    if (!font.header) {
        font.header = (PSF1_HEADER *)(kmalloc(sizeof(PSF1_HEADER)));
        if (!font.header) {
            kloge("INIT PSF: Failed to allocate PSF1 header!\n");
            return PSF1_FAIL;
        }
    }

    font.header->magic = h->magic;
    font.header->font_mode = h->font_mode;
    font.header->character_size = h->character_size;

    if (!PSF1_MAGIC_CHECK(font.header->magic)) {
        return NOT_PSF1;
    }

    /* Set up the glyph buffer */
    if (psf1_font_mode() == SYS_ERR) {
        return PSF1_FAIL;
    }

    size_t num_glyphs = (font.header->font_mode & PSF1_MODE512) ? 512 : 256;
    size_t buffer_size = num_glyphs * font.header->character_size;

    memcpy(font.glyph_buffer,
           (void *)((uint64_t)file->address + sizeof(PSF1_HEADER)),
           buffer_size);

    return PSF1_SUCCESS;
}

/**
 * @brief Helper to initialize a psf font for the system
 *
 * @param req Request for the file from the bootloader
 * @param path Path of the file in the system image
 */
void psf1_font_init(struct limine_module_request req, const char *path) {
    klogs("INIT PSF: Starting...\n");

    if (!req.response) {
        kloge("psf1 file request is NULL!\n");
        halt();
    }

    /* Initialize font file buffer */
    font.header = (PSF1_HEADER *)(kmalloc(sizeof(PSF1_HEADER)));
    if (!font.header) {
        kloge("INIT PSF: PSF1 font header alloc is NULL!\n");
        return;
    }

    LIMINE_FILE *psf_font_file;
    get_iso_file(path, req, &psf_font_file);

    if (!psf_font_file) {
        kloge("The returned struct from get_iso_file is NULL!\n");
        halt();
    }

    if (psf1_get_glyphs(psf_font_file) != PSF1_SUCCESS) {
        kloge("Failed to get glyphs from PSF1 font file!\n");
        halt();
    }

    klogi("Successfully initialized %s\n", path);
    klogs("INIT PSF: finished...\n");
}
