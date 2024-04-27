#include <stdint.h>

/* ------------------ PSF1 ------------------ */
/* All .psf1 glyphs are 8 pixels wide */
typedef struct {
    uint16_t magic; // Magic bytes for identification.
    uint8_t font_mode; // PSF font mode.
    uint8_t character_size; // PSF character size.
} PSF1_HEADER;

typedef struct {
    PSF1_HEADER *header;
    void *glyph_buffer;
} PSF1_FONT;


/* ------------------ PSF2 ------------------ */
typedef struct {
    uint32_t magic;
    uint32_t version;         /* zero (current newest version) */
    uint32_t header_size;     /* offset of bitmaps in file */
    uint32_t flags;           /* 0 if there's no unicode table */
    uint32_t num_glyphs;      /* number of glyphs */
    uint32_t bytes_per_glyph; /* size of each glyph */
    uint32_t height;          /* height in pixels */
    uint32_t width;           /* width in pixels */
} PSF2_FONT;