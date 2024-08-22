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
    font.glyph_buffer = (void *)(kmalloc(256));

    /* TODO: Find a better way of dealing with how big font buffers need to be */
    #if 0
    if (font.header->font_mode & PSF1_MODE512) {
        if (!font.glyph_buffer) {
            kloge("INIT PSF: PSF1 glyph buffer alloc is NULL!\n");
            return SYS_ERR;
        }
        return SYS_OK;
    }
    kloge("INIT PSF: PSF1 font mode not supported!\n");
    return SYS_ERR;
    #endif
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
    font.glyph_buffer = (void *)((uint64_t)file->address + sizeof(PSF1_HEADER));
    return PSF1_SUCCESS;
}

/**
 * @brief Helper to initialize a psf font for the system
 *
 * @param req Request for the file from the bootloader
 * @param path Path of the file in the system image
 */
void psf1_font_init(struct limine_module_request req, const char *path) {
    klogi("INIT PSF: Starting...\n");

    if (!req.response) {
        kloge("psf1 file request is NULL!\n");
        halt();
    }

    /* initialize font file buffer */
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
    klogi("INIT PSF: finished...\n");
}
