#pragma once

#include <stdint.h>

typedef struct {
    const char *name;
    void (*initialize)(uint32_t);
    void (*set_event)(unsigned long);
} PIT_DRIVER;

const PIT_DRIVER *i8253_get_driver();