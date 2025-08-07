/**
 * @file toupper.c
 * @author Zack Bostock
 * @brief Convert to uppercase
 * 
 * @details
 * This implementation is adapted in part from musl libc
 * (https://musl.libc.org/), which is licensed under the MIT license.
 * 
 * @copyright
 * Copyright (c) 2025 Zack Bostock
 * Portions Copyright (c) 2005-2024 Rich Felker, et al.
 *
 * @license
 * This file contains code derived from musl libc, which is licensed
 * under the MIT license. The full license text is provided in the
 * LICENSE file distributed with this software.
 */

#include <ctype.h>

/**
 * @brief Convert to uppercase
 *
 * @param c Character to convert
 * @return int Converted character
 */
int toupper(int c) {
    if (islower(c)) {
        return c & 0x5f;
    }
    return c;
}