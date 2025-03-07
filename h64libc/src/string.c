/**
 * @file string.c
 * @author Zack Bostock
 * @brief Defines functionality for manipulating arrays of characters
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <libc/string.h>
#include <libc/ctype.h>

/**
 * @brief Copies memory from one location to another.
 *
 * @param dst Dest pointer
 * @param src Source pointer
 * @param num Size of memory to copy
 * @return void* Destination where memory was copied to
 */
void *memcpy(void *dst, const void *src, size_t num) {
  uint8_t *u8Dst = (uint8_t *)dst;
  const uint8_t *u8Src = (const uint8_t *)src;
  for (size_t i = 0; i < num; i++) {
    u8Dst[i] = u8Src[i];
  }
  return dst;
}

/**
 * @brief
 *
 * @param ptr Memory location to start from
 * @param value Value to set it to
 * @param num Number of bytes to set
 * @return void* Pointer to the memory which was set
 */
void *memset(void *ptr, int value, size_t num) {
  uint8_t *u8Ptr = (uint8_t *)ptr;
  for (size_t i = 0; i < num; i++) {
    u8Ptr[i] = (uint8_t)value;
  }
  return ptr;
}

/**
 * @brief Compares to pieces of memory to another
 *
 * @param ptr1 First memory location to compare to
 * @param ptr2 Second memory location
 * @param num Number of bytes to compare
 * @return int 1 if different, 0 if not
 */
int memcmp(const void *ptr1, const void *ptr2, size_t num) {
  register const uint8_t *u8Ptr1 = (const uint8_t *)ptr1;
  register const uint8_t *u8Ptr2 = (const uint8_t *)ptr2;

  for (size_t i = 0; i < num; i++) {
    if (u8Ptr1[i] != u8Ptr2[i]) {
      return 1;
    }
  }
  return 0;
}

/**
 * @brief Moves memory from one place to another.
 *
 * @param dest Destination of where to move to
 * @param src Source of where the data is
 * @param n Number of bytes to move
 * @return void* Pointer to the destination
 */
void *memmove(void *dest, const void *src, size_t n) {
  uint8_t *pdest = (uint8_t *)dest;
  const uint8_t *psrc = (const uint8_t *)src;
  if (src > dest) {
    for (size_t i = 0; i < n; i++) {
      pdest[i] = psrc[i];
    }
  } else if (src < dest) {
    for (size_t i = n; i > 0; i--) {
      pdest[i - 1] = psrc[i - 1];
    }
  }
  return dest;
}


/**
 * @brief string copying function for a specific number of bytes
 *
 * @param destination destination in memory to copy to
 * @param src source string to copy
 * @param num number of bytes to copy
 * @return char * returns a pointer to the destination string dest
 */
char *strncpy(char *dest, const char *src, register size_t num) {
    if (!dest) {
        return NULL;
    }

    size_t i;

    for (i = 0; i < num && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < num; i++) {
        dest[i] = '\0';
    }
    return dest;
}

/**
 * @brief Finds the length of a string using the null terminator
 *
 * @param str String to find length of
 * @return size_t Length of string based on null terminator
 */
size_t strlen(const char *str) {
    const char *s = str;
    while (*s) {
        ++s;
    }
    return s - str;
}

/**
 * @brief Compares two strings, stopping after n bytes
 *
 * @param s1 String 1 to compare
 * @param s2 String 2 to compare
 * @param n Number of bytes to compare
 * @return int Returns 0 if same, non-zero if not
 */
int strncmp(const char *s1, const char *s2, register size_t n) {
    register unsigned char u1, u2;
    while (n-- > 0) {
      u1 = (unsigned char) *s1++;
      u2 = (unsigned char) *s2++;
      if (u1 != u2) {
        return u1 - u2;
      }
      if (u1 == '\0') {
        return 0;
      }
    }
  return 0;
}

/**
 * @brief Compares two strings, stopping after hitting null byte
 *
 * @param s1 String 1 to compare
 * @param s2 String 2 to compare
 * @return int Returns 0 if same, non-zero if not
 */
int strcmp(const char *s1, const char *s2) {
    for (size_t i = 0;; i++) {
        if (s1[i] != s2[i] || s1[i] == '\0' || s2[i] == '\0') {
            return s1[i] - s2[i];
        }
    }
    return 0;
}

/**
 * @brief Copies a string into a destination string
 *
 * @param dest String to copy into
 * @param src String to copy from
 * @return int Pointer to the destination string
 */
char *strcpy(char *__restrict dest, const char *src) {
    size_t i;
    for (i = 0;; i++) {
        dest[i] = src[i];
        if (src[i] == '\0') {
            break;
        }
    }
    return dest + i;
}

/**
 * @brief Finds the first occurance of a character in a string
 *
 * @param s String to search through
 * @param c Character to find
 * @return char* Pointer to the character, NULL if not found
 */
char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == c) {
            return (char *) s;
        }
        s++;
    }

    return (char *) NULL;
}

/**
 * @brief Concatenates source string onto destination string
 * @verbatim
 * The  strcat()  function appends the src string to the dest string,
 * overwriting the terminating null byte ('\0') at the end of dest,
 * and then adds a terminating null  byte. The strings  may  not overlap,
 * and the dest string must have enough space for the result. If
 * dest is not large enough, program behavior is unpredictable. Dest must be
 * strlen(dest) + strlen(src) + 1 bytes long to accomidate.
 *
 * @param dest String to concatenate onto
 * @param src String to concatenate from
 * @return int
 */
char *strcat(char *dest, const char *src) {
    size_t i, len = strlen(dest);
    for (i = len;; i++) {
        dest[i] = src[i - len];
        if (src[i - len] == '\0') {
            break;
        }
    }
    return dest;
}

/**
 * @brief Concatenates source string onto destination string, up to n bytes
 *
 * @param dest String to concatenate onto
 * @param src String to concatenate from
 * @param n Number of bytes to concatenate
 * @return char* Pointer to the newly concatenated string
 */
char *strncat(char *dest, const char *src, size_t n) {
    size_t dest_len = strlen(dest);
    size_t i;

    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[dest_len + i] = src[i];
    }
    dest[dest_len + i] = '\0';

    return dest;
}

#ifndef ULONG_MAX
#define	ULONG_MAX	((unsigned long)(~0L))		/* 0xFFFFFFFF */
#endif

#ifndef LONG_MAX
#define	LONG_MAX	((long)(ULONG_MAX >> 1))	/* 0x7FFFFFFF */
#endif

#ifndef LONG_MIN
#define	LONG_MIN	((long)(~LONG_MAX))		/* 0x80000000 */
#endif

/**
 * @brief Convert a string into a long integer
 * @ref https://github.com/gcc-mirror/gcc/blob/master/libiberty/strtol.c
 *
 * @param nptr String
 * @param endptr address of first invalid character
 * @param base between 2-36 inclusive
 * @return long Result of the conversion
 */
long strtol(const char *nptr, char **endptr, register int base) {
    register const char *s = nptr;
    register unsigned long acc;
    register int c;
    register unsigned long cutoff;
    register int neg = 0, any, cutlim;

    /* Skip whitespace and pick up leading +/- sign, if any */
    /* If base is 0, allow 0x for hex and 0 for octal, else assume decimal; */
    /* if base is already 16, allow 0x */

    do {
        c = *s++;
    } while (c == ' ');

    if (c == '-') {
        neg = 1;
        c = *s++;
    } else if (c == '+') {
        c = *s++;
    }
    if ((base == 0 || base == 16) && c == '0' && (*s == 'x' || *s == 'X')) {
        c = s[1];
        s += 2;
        base = 16;
    }

    if (base == 0) {
        base = c == '0' ? 8 : 10;
    }

    cutoff = neg ? -(unsigned long)LONG_MIN : LONG_MAX;
    cutlim = cutoff % (unsigned long) base;
    cutoff /= (unsigned long) base;
    for (acc = 0, any = 0;; c = *s++) {
        if (isdigit(c)) {
            c -= '0';
        } else if (isalpha(c)) {
            c -= isupper(c) ? 'A' - 10 : 'a' - 10;
        } else {
            break;
        }

        if (c >= base) {
            break;
        }
        if (any < 0 || acc < cutoff || (acc == cutoff && c > cutlim)) {
            any = -1;
        } else {
            any = 1;
            acc *= base;
            acc += c;
        }
    }

    if (any < 0) {
        acc = neg ? LONG_MIN : LONG_MAX;
        /* Should set errno to ERANGE here */
    } else if (neg) {
        acc = -acc;
    }
    if (endptr != 0) {
        *endptr = (char *)(any ? s - 1 : nptr);
    }

    return (acc);
}

#ifdef LONG_MIN
#undef LONG_MIN
#endif

#ifdef LONG_MAX
#undef LONG_MAX
#endif

#ifdef ULONG_MAX
#undef ULONG_MAX
#endif