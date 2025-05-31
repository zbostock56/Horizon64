/**
 * @file strerror.c
 * @author Zack Bostock
 * @brief Translates errno's to error strings
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "src/errno/__strerror.h"

char *strerror(int errnum) {
    /* TODO: Check if error messages actually has a error for that errnum */
    return sys_errlist[errnum];
}