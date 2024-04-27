#pragma once

#include <globals.h>

/* ------------------ PSF1 ------------------ */
/* Little-endian */
#define PSF1_MAGIC0      (0x36)
#define PSF1_MAGIC1      (0x04)

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

/* PSF2 font flag bits */
#define PSF2_HAS_UNICODE_TABLE (0x01)

/* UTF8 Separators */
#define PSF2_SEPARATOR         (0xFF)
#define PSF2_STARTSEQ          (0xFE)

#define PSF2_MAXVERSION        (0x00)

/* Header checks */
#define PSF1_MAGIC_CHECK(x) ((((x) & (0xFF)) == PSF1_MAGIC0) && \
                            (((x) & (0xFF00)) == PSF1_MAGIC1))

#define PSF2_MAGIC_CHECK(x) { \
    (((x) & (0xFF)) == PSF2_MAGIC0) && (((x) & (0xFF00)) == PSF2_MAGIC1) &&        \
    (((x) & (0xFF0000)) == PSF2_MAGIC2) && (((x) & (0xFF000000)) == PSF2_MAGIC3)   \
}

/* --------------------------- INTERNALLY DEFINED --------------------------- */

/* --------------------------- EXTERNALLY DEFINED --------------------------- */
