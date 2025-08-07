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
#ifndef NULL
#if __cplusplus >= 201103L
#define NULL nullptr
#elif defined(__cplusplus)
#define NULL 0L
#else
#define NULL ((void*)0)
#endif
#endif

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void   *memcpy  (void *__restrict dest, const void *__restrict src, size_t n);
void   *memmove (void *dest, const void *src, size_t n);
void   *memset  (void *s, int c, size_t n);
int     memcmp  (const void *s1, const void *s2, size_t n);
void   *memchr  (const void *s, int c, size_t n);

char   *strcpy  (char *__restrict dest, const char *__restrict src);
char   *strncpy (char *__restrict dest, const char *__restrict src, size_t n);

char   *strcat  (char *__restrict dest, const char *__restrict src);
char   *strncat (char *__restrict dest, const char *__restrict src, size_t n);

int     strcmp  (const char *s1, const char *s2);
int     strncmp (const char *s1, const char *s2, size_t n);

int     strcoll (const char *s1, const char *s2);
size_t  strxfrm (char *__restrict dest, const char *__restrict src, size_t n);

char   *strchr  (const char *s, int c);
char   *strrchr (const char *s, int c);

size_t  strcspn (const char *s1, const char *s2);
size_t  strspn  (const char *s1, const char *s2);
char   *strpbrk (const char *s1, const char *s2);
char   *strstr  (const char *haystack, const char *needle);
char   *strtok  (char *__restrict str, const char *__restrict delim);

size_t  strlen  (const char *s);
size_t  strnlen (const char *s, size_t n);

char   *strerror(int errnum);