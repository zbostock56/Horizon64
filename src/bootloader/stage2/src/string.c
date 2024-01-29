#include "../include/string.h"
#include "../include/stdint.h"

const char *strchr(const char *str, char c) {
    if (!str) {
        return NULL;
    }
    while (*str) {
        if (*str == c) {
            return str;
        }
        str++;
    }
    return NULL;
}

char *strcpy(char *dst, const char *src) {
    char *original = dst;
    if (dst == NULL) {
        return NULL;
    }
    if (src == NULL) {
        *dst = '\0';
        return dst;
    }
    while (*src) {
        *dst = *src;
        src++;
        dst++;
    }
    *dst = '\0';
    return original;
}

unsigned strlen(const char *str) {
    unsigned len = 0;
    while (*str) {
        len++;
        str++;
    }
    return len;
}