/**
 * @file math.h
 * @author Zack Bostock
 * @brief Common math functionality which is used in various places
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <stdint.h>

/* Avoids the issues with typing */
static inline int MAX(int a, int b) {
    return a > b ? a : b;
}

/* Avoids the issues with typing */
static inline int MIN(int a, int b) {
    return a > b ? b : a;
}

#define ROUND_DOWN(a, b)    ((typeof(a))((a) - ((a) % (b))))
#define ROUND_UP(a, b)      (ROUND_DOWN((a) + (b) - 1, b))
#define DIV_ROUNDUP(a, b)   (((a) + ((b) - 1)) / (b))
#define ALIGNUP(a, b)       (DIV_ROUNDUP(a, b) * (b))

static inline int CLAMP(int val, int min, int max) {
    if (MIN(val, min) == min) {
        return min;
    } else if (MAX(val, max) == max) {
        return max;
    } else {
        return val;
    }
}

