#pragma once

/* 0x00000000 - 0x000003FF : interrupt vector table */
/* 0x00000400 - 0x000004FF : BIOS data area */

#define MEMORY_MIN                  (0x00000500)
#define MEMORY_MAX                  (0x00080000)

/* 0x00000500 - 0x000105000 : FAT Driver */

#define MEMORY_FAT_ADDR    ((void *) 0x00020000)
#define MEMORY_FAT_SIZE             (0x00010000)

#define MEMORY_ELF_ADDR    ((void *) 0x00030000)
#define MEMORY_ELF_SIZE             (0x00010000)

#define MEMORY_LOAD_KERNEL ((void *) 0x00040000)
#define MEMORY_LOAD_SIZE            (0x00010000)

/* 0x00020000 - 0x00030000 : Stage 2 Bootloader */
/* 0x00030000 - 0x00080000 : Free Memory */
/* 0x00080000 - 0x0009FFFF : Extended BIOS data area */
/* 0x000A0000 - 0x000C7FFF : Video Memory */
/* 0x000CB000 - 0x000FFFFF : BIOS */

#define MEMORY_KERNEL_ADDR ((void *) 0x00100000)