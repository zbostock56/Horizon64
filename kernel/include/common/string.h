/**
 * @file string.h
 * @author Zack Bostock
 * @brief Information pertaining to string operations
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <stddef.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
char *strncpy(char *destination, const char *source, size_t num);
size_t strlen(const char *str);
