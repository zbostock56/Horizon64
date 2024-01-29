#include <stdint.h>
#include "gdt.h"

/*
    GDT_ENTRY
    Limit (bits 0 - 15)
    Base (bits 0 - 15)
    Base (bits 16 - 32)
    Access Byte
    Limit (bits 16 - 19) OR flags
    Base (bits 24 - 31)
*/

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t flags_limit_high;
    uint8_t base_high;
} __attribute__((packed)) GDT_ENTRY;

/*
    GDT_DESCRIPTOR
    Size of the GDT - 1
    Address of where the GDT is stored
*/

typedef struct {
    uint16_t limit;
    GDT_ENTRY *entry;
} __attribute__((packed)) GDT_DESCRIPTOR;

typedef enum {
    GDT_FLAG_64BIT                      = 0x20,
    GDT_FLAG_32BIT                      = 0x40,
    GDT_FLAG_16BIT                      = 0x00,

    GDT_FLAG_GRANULARITY_1B             = 0x00,
    GDT_FLAG_GRANULARITY_4K             = 0x80,
} GDT_FLAGS;

typedef enum {
    GDT_ACCESS_CODE_READABLE            = 0x02,
    GDT_ACCESS_DATA_WRITEABLE           = 0x02,

    GDT_ACCESS_CODE_CONFORMING          = 0x04,
    
    GDT_ACCESS_DATA_DIRECTION_DOWN      = 0x04,
    GDT_ACCESS_DATA_DIRECTION_UP        = 0x00,

    GDT_ACCESS_DATA_SEGMENT             = 0x10,
    GDT_ACCESS_CODE_SEGMENT             = 0x18,

    GDT_ACCESS_DESCRIPTOR_TSS           = 0x00,

    GDT_ACCESS_RING0                    = 0x00,
    GDT_ACCESS_RING1                    = 0x20,
    GDT_ACCESS_RING2                    = 0x40,
    GDT_ACCESS_RING3                    = 0x60,

    GDT_ACCESS_PRESENT                  = 0x80,
} GDT_ACCESS_BYTE;

#define GDT_LIMIT_LOW(limit)                (limit & 0xFFFF)
#define GDT_BASE_LOW(base)                  (base & 0xFFFF)
#define GDT_BASE_MID(base)                  ((base >> 16) & 0xFF)
#define GDT_FLAGS_LIMIT_HIGH(limit, flags)  (((limit >> 16) & 0xF) | (flags & 0xF0))
#define GDT_BASE_HIGH(base)                 ((base >> 24) & 0xFF)

#define ENTRY(base, limit, access, flags) {     \
    GDT_LIMIT_LOW(limit),                       \
    GDT_BASE_LOW(base),                         \
    GDT_BASE_MID(base),                         \
    access,                                     \
    GDT_FLAGS_LIMIT_HIGH(limit, flags),         \
    GDT_BASE_HIGH(base)                         \
}

GDT_ENTRY g_gdt[] = {
    /* NULL Descriptor */
    ENTRY(0, 0, 0, 0),
    
    /* 32 Bit Code (ring0) */
    ENTRY(
        0,
        0xFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_CODE_SEGMENT | GDT_ACCESS_CODE_READABLE,
        GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K
    ),
    
    /* 32 Bit Data (ring0) */
    ENTRY(
        0,
        0xFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DATA_SEGMENT | GDT_ACCESS_DATA_WRITEABLE,
        GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K
    ),

    /* 32 Bit Code (ring3) */
    // ENTRY(
    //     0,
    //     0xFFFFF,
    //     GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_CODE_SEGMENT | GDT_ACCESS_CODE_READABLE,
    //     GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K
    // ),
    
    /* 32 Bit Data (ring3) */
    // ENTRY(
    //     0,
    //     0xFFFFF,
    //     GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_DATA_SEGMENT | GDT_ACCESS_DATA_READABLE,
    //     GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K
    // ),
};

GDT_DESCRIPTOR g_gdt_descriptor = {
    sizeof(g_gdt) - 1, 
    g_gdt
};

/* Assembly function which carries out loading the GDT into memory */
void __attribute__((cdecl)) i686_gdt_load(GDT_DESCRIPTOR *, uint16_t, uint16_t);

void i686_gdt_init() {
    i686_gdt_load(&g_gdt_descriptor, i686_GDT_CODE_SEGMENT_RING0, i686_GDT_DATA_SEGMENT_RING0);
}