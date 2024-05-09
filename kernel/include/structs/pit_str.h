#pragma once

#include <stdint.h>

typedef struct {
    const char *name;
    void (*initialize)(uint32_t);
    void (*set_event)(unsigned long);
    unsigned int (*read_frequency)();
} PIT_DRIVER;