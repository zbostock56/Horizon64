/**
 * @file ivec2.c
 * @author Zack Bostock
 * @brief Helpers for the IVEC2 type
 * @verbatim
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <graphics/graphics.h>

/**
 * @brief Scales an IVEC2 by a scalar
 *
 * @param dest ivec2 to output into
 * @param scale amount to scale by
 * @param source ivec2 to operate on
 */
void ivec2_scale(IVEC2 dest, int scale, IVEC2 source) {
  dest.x = scale * source.x;
  dest.y = scale * source.y;
}
