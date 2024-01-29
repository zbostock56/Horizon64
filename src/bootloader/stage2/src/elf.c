/* More information at: https://wiki.osdev.org/ELF */

#include "../include/elf.h"
#include "../include/fat.h"
#include "../include/memdefs.h"
#include "../include/memory.h"
#include "../include/utility.h"
#include "../include/stdio.h"

#ifdef DEBUG
void print_elf_header(ELF_HEADER *header) {
    printf("---- Printing ELF Header ----\n");
    /* Magic number */
    printf("header->magic: [%x, %x, %x, %x]\n", header->magic[0], header->magic[1],
                                                header->magic[2], header->magic[3]);
    if (memcmp(header->magic, ELF_MAGIC, 4) != 0) {
        printf("ERR: header->magic is not %s\n", ELF_MAGIC);
    }

    /* Bitness */
    if (header->bitness == ELF_BITNESS_32BIT) {
        printf("header->bitness = ELF_BITNESS_32BIT\n");
    } else if (header->bitness == ELF_BITNESS_64BIT) {
        printf("header->bitness = ELF_BITNESS_64BIT\n");
    } else {
        printf("ERR: header->bitness = UNKNOWN (%d)\n", header->bitness);
    }

    /* endianness */
    if (header->endianness == ELF_ENDIANNESS_BIG) {
        printf("header->endianness = ELF_ENDIANNESS_BIG\n");
    } else if (header->endianness == ELF_ENDIANNESS_LITTLE) {
        printf("header->endianness = ELF_ENDIANNESS_LITTLE\n");
    } else {
        printf("ERR: header->endianness = UNKNOWN (%d)\n", header->endianness);
    }

    /* ELF header version */
    if (header->ELF_header_version == 1) {
        printf("header->ELF_header_version = OK (1)\n");
    } else {
        printf("header->ELF_header_version = UNKNOWN (%d)\n", header->ELF_header_version);
    }

    /* ELF version */
    if (header->ELF_version == 1) {
        printf("header->ELF_version = OK (1)\n");
    } else {
        printf("header->ELF_version = UNKNOWN (%d)\n", header->ELF_version);
    }

    /* Type */
    if (header->type == ELF_TYPE_CORE) {
        printf("header->type = ELF_TYPE_CORE\n");
    } else if (header->type == ELF_TYPE_EXECUTABLE) {
        printf("header->type = ELF_TYPE_EXECUTABLE\n");
    } else if (header->type == ELF_TYPE_RELOCATABLE) {
        printf("header->type = ELF_TYPE_RELOCATABLE\n");
    } else if (header->type == ELF_TYPE_SHARED) {
        printf("header->type = ELF_TYPE_SHARED\n");
    } else {
        printf("ERR: header->type = UNKNOWN (%d)\n", header->type);
    }

    /* ISA */
    if (header->instruction_set == ELF_INSTRUCTION_SET_NONE) {
        printf("header->instruction_set = ELF_INSTRUCTION_SET_NONE\n");
    } else if (header->instruction_set == ELF_INSTRUCTION_SET_X86) {
        printf("header->instruction_set = ELF_INSTRUCTION_SET_X86\n");
    } else if (header->instruction_set == ELF_INSTRUCTION_SET_ARM) {
        printf("header->instruction_set = ELF_INSTRUCTION_SET_ARM\n");
    } else if (header->instruction_set == ELF_INSTRUCTION_SET_X64) {
        printf("header->instruction_set = ELF_INSTRUCTION_SET_X64\n");
    } else if (header->instruction_set == ELF_INSTRUCTION_SET_ARM64) {
        printf("header->instruction_set = ELF_INSTRUCTION_SET_ARM64\n");
    } else if (header->instruction_set == ELF_INSTRUCTION_SET_RISCV) {
        printf("header->instruction_set = ELF_INSTRUCTION_SET_RISCV\n");
    } else {
        printf("header->instruction_set = UNKNOWN (%d)\n", header->instruction_set);
    }
}
#endif

bool ELF_read(PARTITION *part, const char *path, void **entry_point) {
    uint8_t *header_buffer = MEMORY_ELF_ADDR;
    uint8_t *load_buffer = MEMORY_LOAD_KERNEL;
    uint32_t file_pos = 0;
    uint32_t read;

    /* Read header */
    FAT_FILE *fd = FAT_open(part, path);
    if ((read = FAT_read(part, fd, sizeof(ELF_HEADER), header_buffer))
        != sizeof(ELF_HEADER)) {
        printf("ELF load error!\n");
        return false;
    }

    file_pos += read;

    /* Validate header */
    bool ok = true;
    ELF_HEADER *header = (ELF_HEADER *) header_buffer;
    //ok = ok && (memcmp(header->magic, ELF_MAGIC, 4) != 0);
    ok = ok && (header->bitness == ELF_BITNESS_32BIT);
    ok = ok && (header->endianness == ELF_ENDIANNESS_LITTLE);
    ok = ok && (header->ELF_header_version == 1);
    ok = ok && (header->ELF_version == 1);
    ok = ok && (header->type == ELF_TYPE_EXECUTABLE);
    ok = ok && (header->instruction_set == ELF_INSTRUCTION_SET_X86);

    if (!ok) {
        printf("ELF header validation failed!\n");
        #if DEBUG
        print_elf_header(header);
        #endif
        return false;
    }

    *entry_point = (void *) header->program_entry_position;

    /* Load program header */
    uint32_t program_header_offset = header->program_header_table_position;
    uint32_t program_header_size = header->program_header_table_entry_size *
                                   header->program_header_table_entry_count;
    uint32_t program_header_table_entry_size = header->program_header_table_entry_size;
    uint32_t program_header_table_entry_count = header->program_header_table_entry_count;

    file_pos += FAT_read(part, fd, program_header_offset - file_pos, header_buffer);
    if ((read = FAT_read(part, fd, program_header_size, header_buffer)) 
                        != program_header_size) {
        printf("ELF load error!\n");
        return false;
    }

    file_pos += read;
    FAT_close(fd);

    /* Parse program header entries */
    for (uint32_t i = 0; i < program_header_table_entry_count; i++) {
        ELF_PROGRAM_HEADER *prog_header = (ELF_PROGRAM_HEADER *) (header_buffer +
                                           i * program_header_table_entry_size);
        if (prog_header->type == ELF_PROGRAM_TYPE_LOAD) {
            uint8_t *virt_addr = (uint8_t *) prog_header->virtual_address;
            memset(virt_addr, 0, prog_header->memory_size);

            fd = FAT_open(part, path);
            while (prog_header->offset > 0) {
                uint32_t should_read = min(prog_header->offset, MEMORY_LOAD_SIZE);
                read = FAT_read(part, fd, should_read, load_buffer);
                if (read != should_read) {
                    printf("ELF load error!\n");
                    return false;
                }
                prog_header->offset -= read;
            }

            /* Read program */
            while (prog_header->file_size > 0) {
                uint32_t should_read = min(prog_header->file_size, MEMORY_LOAD_SIZE);
                read = FAT_read(part, fd, should_read, load_buffer);
                if (read != should_read) {
                    printf("ELF load error!\n");
                    return false;
                }
                prog_header->file_size -= read;

                memcpy(virt_addr, load_buffer, read);
                virt_addr += read;
            }
            FAT_close(fd);
        }
    }
    return true;
}