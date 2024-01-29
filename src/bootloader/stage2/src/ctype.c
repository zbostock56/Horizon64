#include "../include/ctype.h"

uint8_t islower(char c) {
    return c >= 'a' && c <= 'z';
}

char toupper(char c) {
    return islower(c) ? (c - 'a' + 'A'): c;
}