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
 * @brief Gets the glyphs from a .psf type file
 *
 * @param file File of glyphs
 * @return int Success or failure status
 */
int psf1_get_glyphs(LIMINE_FILE *file) {
  font.header = (PSF1_HEADER *) file->address;
  if (!PSF1_MAGIC_CHECK(font.header->magic)) {
    return NOT_PSF1;
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

  if (!req.response) {
    kloge("psf1 file request is NULL!\n");
    halt();
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
}
