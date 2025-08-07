/**
 * @file strtok.c
 * @author Zack Bostock
 * @brief Breaks strings into tokens separated by delimiters
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <string.h>

static char *strtok_ptr = NULL;

/**
 * @brief Breaks string into tokens separated by delimiters.
 *
 * @param str String to tokenize (NULL to continue with previous string)
 * @param delim String containing delimiter characters
 * @return char* Pointer to next token, or NULL if no more tokens
 */
char *strtok(char *__restrict str, const char *__restrict delim) {
    char *token_start;

    if (str != NULL) {
        strtok_ptr = str;
    } else if (strtok_ptr == NULL) {
        return NULL;
    }

    /* Skip leading delimiters */
    strtok_ptr += strspn(strtok_ptr, delim);

    if (*strtok_ptr == '\0') {
        return NULL;
    }

    token_start = strtok_ptr;

    /* Find end of token */
    strtok_ptr += strcspn(strtok_ptr, delim);

    if (*strtok_ptr != '\0') {
        *strtok_ptr++ = '\0';
    }

    return token_start;
}