/**
 * @file string.h
 * @author Zack Bostock
 * @brief Information pertaining to string operations
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void *memcpy(void *dst, const void *src, size_t num);
void *memset(void *ptr, int value, size_t num);
int memcmp(const void *ptr1, const void *ptr2, size_t num);
void *memmove(void *dest, const void *src, size_t n);
char *strncpy(char *destination, const char *source, register size_t num);
char *strcpy(char *__restrict dest, const char *src);
size_t strlen(const char *str);
int strncmp(const char *s1, const char *s2, register size_t n);
int strcmp(const char *s1, const char *s2);
char *strchr(const char *s, int c);
char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, size_t n);
long strtol(const char *nptr, char **endptr, register int base);