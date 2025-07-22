/**
 * @file klib.h
 * @author Zack Bostock
 * @brief Includes functionality which may be used throughout the kernel
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <common/kprint.h>
#include <sys/asm.h>

#define ASSERT(x) {                             \
    if (!(x)) {                                 \
        kloge("%s() ASSERT failed in %s:%d\n",  \
                __func__, __FILE__, __LINE__);  \
        halt();                                 \
    }                                           \
}
