/* More information at: https://wiki.osdev.org/ELF */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "mbr.h"

/* 0x7F ELF*/
#define ELF_MAGIC ("\x7F" "\x45" "\x4c" "\x46")

typedef struct {
    uint8_t magic[4];
    uint8_t bitness;    /* 1 = 32 bit, 2 = 64 bit */
    uint8_t endianness;
    uint8_t ELF_header_version;
    uint8_t ABI;
    uint8_t _padding[8];
    uint16_t type;      /* 1 = relocatable, 2 = exec, 3 = shared, 4 = core*/
    uint16_t instruction_set;
    uint32_t ELF_version;
    uint32_t program_entry_position;
    uint32_t program_header_table_position;
    uint32_t section_header_table_position;
    uint32_t flags;
    uint16_t header_size;
    uint16_t program_header_table_entry_size;
    uint16_t program_header_table_entry_count;
    uint16_t section_header_table_entry_size;
    uint16_t section_header_table_entry_count;
    uint16_t section_names_index;
}__attribute__((packed)) ELF_HEADER;

enum ELF_bitness {
    ELF_BITNESS_32BIT                  = 1,
    ELF_BITNESS_64BIT                  = 2,
};

enum ELF_endianness {
    ELF_ENDIANNESS_LITTLE              = 1,
    ELF_ENDIANNESS_BIG                 = 2,
};

enum ELF_instruction_set {
    ELF_INSTRUCTION_SET_NONE           = 0,
    ELF_INSTRUCTION_SET_X86            = 3,
    ELF_INSTRUCTION_SET_ARM            = 0x28,
    ELF_INSTRUCTION_SET_X64            = 0x3E,
    ELF_INSTRUCTION_SET_ARM64          = 0xB7,
    ELF_INSTRUCTION_SET_RISCV          = 0xF3,
};

enum ELF_type {
    ELF_TYPE_RELOCATABLE               = 1,
    ELF_TYPE_EXECUTABLE                = 2,
    ELF_TYPE_SHARED                    = 3,
    ELF_TYPE_CORE                      = 4,
};

typedef struct {
    uint32_t type;
    uint32_t offset;
    uint32_t virtual_address;
    uint32_t physical_address;
    uint32_t file_size;
    uint32_t memory_size;
    uint32_t flags;
    uint32_t align;
} ELF_PROGRAM_HEADER;

enum ELF_program_type {
    /* Program header table entry (unused) */
    ELF_PROGRAM_TYPE_NULL              = 0,
    /* Loadable segment */
    ELF_PROGRAM_TYPE_LOAD              = 1,
    /* Dynamic linking info */
    ELF_PROGRAM_TYPE_DYNAMIC           = 2,
    /* Interpreter information */
    ELF_PROGRAM_TYPE_INTERP            = 3,
    /* Auxiliary info */
    ELF_PROGRAM_TYPE_NOTE              = 4,
    /* Reserved */
    ELF_PROGRAM_TYPE_SHLIB             = 5,
    /* Segment containing program header table itself */
    ELF_PROGRAM_TYPE_PHDR              = 6,
    /* Thread local storage template */
    ELF_PROGRAM_TYPE_TLS               = 7,
    /* Reserved inclusive range (OS-specific) */
    ELF_PROGRAM_TYPE_LOOS              = 0x60000000,
    ELF_PROGRAM_TYPE_HIOS              = 0x6FFFFFFF,
    /* Reserved inclusive range (Processor specific) */
    ELF_PROGRAM_TYPE_LOPROC            = 0x70000000,
    ELF_PROGRAM_TYPE_HIPROC            = 0x7FFFFFFF,
};

bool ELF_read(PARTITION *, const char *, void **);