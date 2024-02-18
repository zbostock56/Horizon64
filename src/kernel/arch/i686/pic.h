#pragma once

#include <stdint.h>

typedef struct {
    const char *name;
    int (*probe)();
    void (*initialize)(uint8_t, uint8_t);
    void (*disable)();
    void (*send_end_of_interrupt)(int);
    void (*mask)(int);
    void (*unmask)(int);
} PIC_DRIVER;

const PIC_DRIVER *i8259_get_driver();