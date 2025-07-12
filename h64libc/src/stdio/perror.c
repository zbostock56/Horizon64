/**
 * @file perror.c
 * @author Zack Bostock
 * @brief Prints a system error message
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <libc/string.h>
#include <libc/stdio.h>

#include "src/internal/__internal.h"

void perror(const char *s) {
    char *errstr = strerror(__set_errno);

    if (s && *s) {
        fprintf(STDERR, "%s: ", s);
    }
    fprintf(STDERR, "%s\n", errstr);
}