/**
 * @file isupper.c
 * @author Zack Bostock
 * @brief Check if a character is uppercase
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
 * @brief Check if a character is uppercase
 *
 * @param c Character to check
 * @return int 1 if true, 0 if false
 */
int isupper(int c) {
    return (unsigned)c - 'A' < 26;
}