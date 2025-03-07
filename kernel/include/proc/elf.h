/**
 * @file elf.h
 * @author Zack Bostock
 * @brief Information pertaining to ELF files
 * @ref https://codebrowser.dev/glibc/glibc/elf/elf.h.html
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

#include <structs/elf_str.h>

#include <sys/mmu.h>

#include <proc/process.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
/**
 * @brief "\x7FELF" in little endian
 */
#define ELF_MAGIC               (0x464C457FU)

/**
 * @brief Object file type
 */
#define ET_NONE                 (0x0)           /* No file type */
#define ET_RELOC                (0x1)           /* Relocable file */
#define ET_EXEC                 (0x2)           /* Executable File */
#define ET_SHARED               (0x3)           /* Shared object file */
#define ET_CORE                 (0x4)           /* Core file */
#define ET_NUM                  (0x5)           /* Number of defined types */

/**
 * @brief Segment type
 */
#define PT_NULL                 (0x00000000)    /* Program header table entry unused */
#define PT_LOAD                 (0x00000001)    /* Loadable program segment */
#define PT_DYNAMIC              (0x00000002)    /* Dynamic linking information */
#define PT_INTERP               (0x00000003)    /* Program interpreter */
#define PT_NOTE                 (0x00000004)    /* Auxiliary information */
#define PT_SHLIB                (0x00000005)    /* Reserved */
#define PT_PHDR                 (0x00000006)    /* Entry for header table itself */
#define PT_TLS                  (0x00000007)    /* Thread-local storage segment */

#define ABI_SYSV                (0x00)

#define ARCH_X86_64             (0x3E)
#define ARCH_AARCH64            (0xB7)
#define ARCH_ARM                (0x28)
#define ARCH_X86                (0x03)

#define BITS_BE                 (0x00)
#define BITS_LE                 (0x01)

// #define EI_CLASS                (0x4)
// #define EI_DATA                 (0x5)
// #define EI_VERSION              (0x6)
// #define EI_OSABI                (0x7)
// #define EI_ABIVERSION           (0x8)

#define EI_CLASS                (0x0)
#define EI_DATA                 (0x1)
#define EI_VERSION              (0x2)
#define EI_OSABI                (0x3)
#define EI_ABIVERSION           (0x4)

/**
 * @brief Segment flags
 */
#define PF_X                    (0x1)           /* Segment is executable */
#define PF_W                    (0x2)           /* Segement is writable */
#define PF_R                    (0x4)           /* Segment is readable */

/**
 * @brief ELF Sections
 */
#define SHT_NULL                (0)             /* Section header table entry unused */
#define SHT_PROGBITS            (1)             /* Program data */
#define SHT_SYMTAB              (2)             /* Symbol table */
#define SHT_STRTAB              (3)             /* String table */
#define SHT_RELA                (4)             /* Relocation entries with addends */
#define SHT_HASH                (5)             /* Symbol hash table */
#define SHT_DYNAMIC             (6)             /* Dynamic linking information */
#define SHT_NOTE                (7)             /* Notes */
#define SHT_NOBITS              (8)             /* Program space with no data (bss) */
#define SHT_REL                 (9)             /* Relation entries, no addends */
#define SHT_SHLIB               (10)            /* Reserved */
#define SHT_DYNSYM              (11)            /* Dynamic linker symbol table */
/* There is an intentional gap here */
#define SHT_INIT_ARRAY          (14)            /* Array of constructors */
#define SHT_FINI_ARRAY          (15)            /* Array of destructors */
#define SHT_PREINIT_ARRAY       (16)            /* Array of pre-constructors */
#define SHT_GROUP               (17)            /* Section group */
#define SHT_SYMTAB_SHNDX        (18)            /* Extended section indices */
#define SHT_NUM                 (19)            /* RELR relative relocations */

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
size_t elf_load(PROCESS *p, const char *path_name, uint64_t *entry_point,
                ELF_AUXVAL *aux);