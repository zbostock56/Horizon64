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
char *strcpy(char *__restrict dest, const char *src);
size_t strlen(const char *str);
int strncmp(const char *s1, const char *s2, register size_t n);
int strcmp(const char *s1, const char *s2);
char *strchr(const char *s, int c);
char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, size_t n);
int toupper(int c);
int isdigit(int c);
int isupper(int c);
int isalpha(int c);
long strtol(const char *nptr, char **endptr, register int base);
char *strrchr(const char *s, int c);