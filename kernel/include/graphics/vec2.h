/**
 * @file vec2.h
 * @author Zack Bostock 
 * @brief Information pertaining to 2D floating-point vectors 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

typedef struct {
    float x;
    float y;
} VEC2;

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
#define VEC2_ZERO_INIT  {0.0f, 0.0f}
#define VEC2_ONE_INIT   {1.0f, 1.0f}

#define VEC2_ZERO       ((VEC2) VEC2_ZERO_INIT)
#define VEC2_ONE        ((VEC2) VEC2_ONE_INIT)

/* ----------------------------- STATIC GLOBALS ----------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */

/* --------------------------- EXTERNALLY DEFINED --------------------------- */