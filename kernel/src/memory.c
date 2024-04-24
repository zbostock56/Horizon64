#include <memory.h>

void *memcpy(void *dst, const void *src, size_t num) {
    uint8_t *u8Dst = (uint8_t *) dst;
    const uint8_t *u8Src = (const uint8_t *) src;
    for (size_t i = 0; i < num; i++) {
        u8Dst[i] = u8Src[i];
    }
    return dst;
}

void *memset(void *ptr, int value, size_t num) {
    uint8_t *u8Ptr = (uint8_t *) ptr;
    for (size_t i = 0; i < num; i++) {
        u8Ptr[i] = (uint8_t) value;
    }
    return ptr;
}

int memcmp(const void *ptr1, const void *ptr2, size_t num) {
    const uint8_t *u8Ptr1 = (const uint8_t *) ptr1;
    const uint8_t *u8Ptr2 = (const uint8_t *) ptr2;

    for (size_t i = 0; i < num; i++) {
        if (u8Ptr1[i] != u8Ptr2[i]) {
            return 1;
        }
    }
    return 0;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *pdest = (uint8_t *) dest;
    const uint8_t *psrc = (const uint8_t *) src;
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