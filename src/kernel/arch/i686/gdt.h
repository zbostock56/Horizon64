#pragma once

#define i686_GDT_CODE_SEGMENT_RING0  (0x08)
#define i686_GDT_DATA_SEGMENT_RING0  (0x10)
#define i686_GDT_CODE_SEGMENT_RING3  (0x18)
#define i686_GDT_DATA_SEGMENT_RING3  (0x20)

void i686_gdt_init();