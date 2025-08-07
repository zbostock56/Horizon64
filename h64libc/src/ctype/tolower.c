/**
 * @file tolower.c
 * @author Zack Bostock
 * @brief Convert to lowercase
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
 * @brief Convert to lowercase
 *
 * @param c Character to convert
 * @return int Converted character
 */
int tolower(int c) {
    if (isupper(c)) {
        return c | 32;
    }
    return c;
}