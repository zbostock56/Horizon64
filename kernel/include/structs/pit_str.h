/**
 * @file pit_str.h
 * @author Zack Bostock 
 * @brief Structs pertaining to the Programmable Interrupt Timer driver 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <stdint.h>

typedef struct {
    const char *name;
    void (*initialize)(uint32_t);
    void (*set_event)(unsigned long);
    unsigned int (*read_frequency)();
} PIT_DRIVER;