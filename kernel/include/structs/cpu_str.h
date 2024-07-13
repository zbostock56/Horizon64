/**
 * @file cpu_str.h
 * @author Zack Bostock 
 * @brief Structs utilized by CPU functionality 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <stdint.h>

/*
  Here, we define the structures necessary to check CPU specific features
  such as APIC, AVX512, etc.
*/

typedef struct {
  uint32_t feature;
  uint32_t param;
  enum {
    CPUID_EAX   = 0x0,
    CPUID_EBX   = 0x1,
    CPUID_ECX   = 0x2,
    CPUID_EDX   = 0x3,
  } registers;
  uint32_t mask;
} CPUID_FEATURE;

typedef struct {
    uint32_t ebx;
    const char *vendor_name;
} VENDOR;