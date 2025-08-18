/**
 * @file string.c
 * @author Zack Bostock
 * @brief Functionality pertaining to string manipulation
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <common/string.h>
#include <stdarg.h>

/**
 * @brief string copying function for a specific number of bytes
 *
 * @param destination destination in memory to copy to
 * @param src source string to copy
 * @param num number of bytes to copy
 * @return char * returns a pointer to the destination string dest
 */
char *strncpy(char *dest, const char *src, size_t num) {
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

/**
 * @brief Converts to an uppercase
 * @note Assumes is character and isn't uppercase already
 * 
 * @param c Character to turn to uppercase
 * @return int Uppercase character
 */
int toupper(int c) {
    return c - 32;
}

/**
 * @brief Simple check for checking if a character is a digit
 *
 * @param c Character to check
 * @return int true if is digit, false otherwise
 */
int isdigit(int c) {
    return (unsigned) c - '0' < 10;
}

/**
 * @brief Simple helper to check if a character is uppercase
 *
 * @param c Character to check
 * @return int True if uppercase, false otherwise
 */
int isupper(int c) {
    return (c >= 'A' && c <= 'Z');
}

/**
 * @brief Simple helper to check if a character is alphanumeric
 *
 * @param c Character to check
 * @return int True if alphanumeric, false otherwise
 */
int isalpha(int c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
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

/**
 * @brief Locate the last occurrence of a character in a string.
 *
 * @param s The string to search.
 * @param c The character to find.
 * @return char* A pointer to the last occurrence of the character,
 *               or NULL if not found.
 */
char *strrchr(const char *s, int c) {
    const char *last = 0;
    while (*s) {
        if (*s == (char)c) {
            last = s;
        }
        s++;
    }
    /* If searching for the null terminator, return a pointer to the */
    /* end of the string                                             */
    if ((char)c == '\0') {
        return (char *)s;
    }
    return (char *)last;
}

/**
 * @brief Tokenizes a string using a set of delimiter characters.
 *
 * This function splits the input string into a sequence of tokens separated
 * by any of the characters in the delimiter string. On the first call,
 * `str` should be the string to tokenize. On subsequent calls to extract
 * additional tokens from the same string, `str` should be NULL.
 *
 * The input string is modified in-place: delimiters are replaced by null
 * terminators (`'\0'`). The function maintains internal state and is
 * therefore not thread-safe.
 *
 * @param str The string to tokenize, or NULL to continue tokenizing the previous string.
 * @param delim A null-terminated string of delimiter characters.
 * @return A pointer to the next token, or NULL if there are no more tokens.
 *
 * @note This function is not thread-safe.
 *
 * @warning The input string must be writable. This function modifies it in-place.
 */

char *strtok(char *str, const char *delim) {
    static char *saved = NULL;

    if (str)
        saved = str;
    else if (!saved)
        return NULL;

    // Skip leading delimiters
    char *token_start = saved;
    while (*token_start && strchr(delim, *token_start)) {
        token_start++;
    }

    if (*token_start == '\0') {
        saved = NULL;
        return NULL;
    }

    // Find the end of the token
    char *token_end = token_start;
    while (*token_end && !strchr(delim, *token_end)) {
        token_end++;
    }

    if (*token_end) {
        *token_end = '\0';
        saved = token_end + 1;
    } else {
        saved = NULL;
    }

    return token_start;
}

static inline void safe_string_copy(char *dest, const char *src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) return;

    strncpy(dest, src, dest_size - 1);  // Leave room for null terminator
    dest[dest_size - 1] = '\0';         // Ensure null termination
}

static inline int int_to_string(unsigned int value, char *str, int base) {
    char *ptr = str;
    char *start = str;
    char temp;

    if (value == 0) {
        *ptr++ = '0';
        *ptr = '\0';
        return 1;
    }

    while (value > 0) {
        int digit = value % base;
        *ptr++ = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
        value /= base;
    }

    int len = ptr - start;
    *ptr = '\0';

    ptr--;
    while (start < ptr) {
        temp = *start;
        *start++ = *ptr;
        *ptr-- = temp;
    }

    return len;
}

int snprintf(char *str, size_t size, const char *format, ...) {
    if (!str || size == 0) return -1;

    va_list args;
    va_start(args, format);

    char *dest = str;
    const char *fmt = format;
    size_t remaining = size - 1;

    while (*fmt && remaining > 0) {
        if (*fmt == '%' && *(fmt + 1)) {
            fmt++;
            switch (*fmt) {
                case 's': {
                    char *s = va_arg(args, char*);
                    while (*s && remaining > 0) {
                        *dest++ = *s++;
                        remaining--;
                    }
                    break;
                }
                case 'd': {
                    int val = va_arg(args, int);
                    char temp[16];
                    int len = int_to_string(val, temp, 10);
                    for (int i = 0; i < len && remaining > 0; i++) {
                        *dest++ = temp[i];
                        remaining--;
                    }
                    break;
                }
                case 'x': {
                    unsigned int val = va_arg(args, unsigned int);
                    char temp[16];
                    int len = int_to_string(val, temp, 16);
                    for (int i = 0; i < len && remaining > 0; i++) {
                        *dest++ = temp[i];
                        remaining--;
                    }
                    break;
                }
                default:
                    if (remaining > 0) {
                        *dest++ = *fmt;
                        remaining--;
                    }
                    break;
            }
        } else {
            *dest++ = *fmt;
            remaining--;
        }
        fmt++;
    }

    *dest = '\0';
    va_end(args);
    return dest - str;
}

/**
 * @brief Finds the length of a string using the null terminator, bounded by len
 *
 * @param str String to find length of
 * @param len Max length to stop at
 * @return size_t Length of string based on null terminator, or limit
 */
size_t strnlen(const char *str, size_t len) {
    const char *s = str;
    for (size_t i = 0; i < len; i++) {
        if (!(*s)) {
            return s - str;
        }
        s++;
    }
    return len;
}
