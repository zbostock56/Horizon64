#include <graphics/graphics.h>

/*
  ivec2 scale
  input:
    - ivec2 destination: ivec2 to output into
    - int scale: amount to scale by
    - ivec2 source: ivec2 to operate on
*/
void ivec2_scale(IVEC2 dest, int scale, IVEC2 source) {
  dest.x = scale * source.x;
  dest.y = scale * source.y;
}