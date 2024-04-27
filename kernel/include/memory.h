#pragma once
#include <stdint.h>
#include <stddef.h>

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void *memcpy(void *, const void *, size_t);
void *memset(void *, int, size_t);
int memcmp(const void *, const void *, size_t);
void *memmove(void *, const void *, size_t);

/* --------------------------- EXTERNALLY DEFINED --------------------------- */
