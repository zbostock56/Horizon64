#pragma once

#include <globals.h>

/* ----------------------------- STATIC GLOBALS ----------------------------- */

/* --------------------------------- DEFINES -------------------------------- */
/* ------------------ PSF1 ------------------ */
/* Little-endian */
#define PSF1_MAGIC0      (0x36)
#define PSF1_MAGIC1      (0x04)

#define PSF1_FULL_MAGIC  (0x0436)

/* PSF1 font mode bits */
#define PSF1_MODE512     (0x01)
#define PSF1_MODEHASTAB  (0x02)
#define PSF1_MODESEQ     (0x04)
#define PSF1_MAXMODE     (0x05)

#define PSF1_SEPARATOR   (0xFFFF)
#define PSF1_STARTSEQ    (0xFFFE)

/* ------------------ PSF2 ------------------ */
/* Little-endian */
#define PSF2_MAGIC0            (0x72)
#define PSF2_MAGIC1            (0xB5)
#define PSF2_MAGIC2	           (0x4A)
#define PSF2_MAGIC3	           (0x86)

#define PSF2_FULL_MAGIC        (0x864ab572)

/* PSF2 font flag bits */
#define PSF2_HAS_UNICODE_TABLE (0x01)

/* UTF8 Separators */
#define PSF2_SEPARATOR         (0xFF)
#define PSF2_STARTSEQ          (0xFE)

#define PSF2_MAXVERSION        (0x00)

/* --------------------------------- MACROS --------------------------------- */
/* Header checks */
#define PSF1_MAGIC_CHECK(x) (((x) & (0xFFFF)) == (PSF1_FULL_MAGIC))
#define PSF2_MAGIC_CHECK(x) ((x) & (0xFFFFFFFF) == (PSF2_FULL_MAGIC))

/* --------------------------- INTERNALLY DEFINED --------------------------- */
int psf1_get_glyphs(LIMINE_FILE *file);
void psf1_font_init(struct limine_module_request req, const char *path);

/* --------------------------- EXTERNALLY DEFINED --------------------------- */
void get_iso_file(const char *name, LIMINE_MODULE_REQ module_request,
                  LIMINE_FILE **file);