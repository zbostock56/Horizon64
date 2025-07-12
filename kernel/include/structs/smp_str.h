/**
 * @file smp_str.h
 * @author Zack Bostock
 * @brief Structures related to Symmetric Multiprocessing (SMP)
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <const.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t reserved_0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;

    uint32_t reserved_1;
    uint32_t reserved_2;

    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;

    uint64_t reserved_3;
    uint64_t reserved_4;

    uint16_t io_bitmap_offset;
} __attribute__((packed)) TSS;

typedef struct {
    int64_t errno;
    TSS tss;
    uint16_t cpu_id;
    uint16_t lapic_id;
    uint8_t is_bootstrap_processor;
    size_t fpu_storage_size;
    void (*fpu_save)(void *);
    void (*fpu_restore)(void *);
    uint16_t proc_id;
    uint8_t reserved;
} CPU;

typedef struct {
    CPU cpus[MAX_CPUS];
    uint16_t num_cpus;
} SMP_INFO;