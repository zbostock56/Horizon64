#include <limine.h>
#include <graphics/graphics.h>

typedef struct {
    void *base;
    uint64_t height;
    uint64_t width;
    uint64_t pitch;
    uint16_t bpp;

    /* Can be either psf1 or psf2 */
    int font_type;
    void *font;
    IVEC2 cursor_pos;
} FRAMEBUFFER;