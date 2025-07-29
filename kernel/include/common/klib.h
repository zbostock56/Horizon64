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
#include <sys/mmu.h>
#include <sys/asm.h>

static inline void __assert_fail(const char *func, const char *file, int line) {
    kloge("%s() ASSERT failed in %s:%d\n", func, file, line);
#if KMEM_DEBUG
    mem_debug();
#endif
    halt();
}

#define ASSERT(x)                                           \
    do {                                                    \
        if (!(x)) {                                         \
            __assert_fail(__func__, __FILE__, __LINE__);    \
        }                                                   \
    } while (0)

