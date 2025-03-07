/**
 * @file elf_str.h
 * @author Zack Bostock
 * @brief Structures pertaining to ELF files
 * @ref https://wiki.osdev.org/ELF_Tutorial
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <stdint.h>

/**
 * @brief Standard ELF file header
 */
typedef struct {
    uint32_t magic;     /* Magic number */
    uint8_t  elf[12];
    uint16_t type;      /* Object file type */
    uint16_t machine;   /* Architecture */
    uint32_t version;   /* Object file version */
    uint64_t entry;     /* Entry point virtual address */
    uint64_t phoff;     /* Program header table file offset */
    uint64_t shoff;     /* Section header table file offset */
    uint32_t flags;     /* Processor-specific flags */
    uint16_t ehsize;    /* ELF header size in bytes */
    uint16_t phentsize; /* Program header table entry size */
    uint16_t phnum;     /* Program header table entry count */
    uint16_t shentsize; /* Section header table entry size */
    uint16_t shnum;     /* Section header table entry count */
    uint16_t shstrndx;  /* Section header string table index */
} ELF_HEADER;

/**
 * @brief ELF Program Header
 */
typedef struct {
    uint32_t type;      /* Segment type */
    uint32_t flags;     /* Segment-dependent flags */
    uint64_t offset;    /* Segment offset in the file image */
    uint64_t vaddr;     /* Segment virtual address in memory */
    uint64_t paddr;     /* Reserved for segment physical address */
    uint64_t filesz;    /* Segment size in file image */
    uint64_t memsz;     /* Segment size in memory */
    uint64_t align;     /* 0 and 1 specify no alignment. Otherwise should be a
                         * positive, integral power of 2, with p_vaddr equating
                         * p_offset modulus p_align.
                         */
} ELF_PHEADER;

/**
 * @brief ELF Section Header
 */
typedef struct {
    uint32_t name;      /* An offset to a string in the shstrtab section */
    uint32_t type;      /* Section header type */
    uint64_t flags;     /* Section attributes */
    uint64_t addr;      /* Virtual address of the section in memory */
    uint64_t offset;    /* Offset of the section in the file image */
    uint64_t size;      /* Size in bytes of the section in the file image */
    uint32_t link;      /* Contain section index of an associated section */
    uint32_t info;      /* Contain extra information about the section */
    uint64_t addralign; /* Contains the required alignment of the section */
    uint64_t entsize;   /* Contains the size, in bytes, of each entry, for
                         * sections that contain fixed-size entries. Otherwise,
                         *  this field contains zero.
                         */
} ELF_SHEADER;

typedef struct {
    uint32_t name_size;
    uint32_t desc_size;
    uint32_t type;
} ELF_NHEADER;

/**
 * @brief ELF Symbol
 */
typedef struct {
    uint32_t name;      /* Symbol name (string tbl index) */
    uint8_t  info;      /* Symbol type and binding */
    uint8_t  other;     /* Symbol visibility (and 0) */
    uint16_t shndx;     /* Section index */
    uint64_t value;     /* Symbol value */
    uint64_t size;      /* Symbol size */
} ELF_SYM;