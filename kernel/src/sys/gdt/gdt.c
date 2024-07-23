/**
 * @file gdt.c
 * @author Zack Bostock
 * @brief Functions pertaining to the Global Descriptor Table
 * @verbatim
 * In this file are the initialization functions associated with the Global
 * Descriptor Table (GDT).
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <sys/gdt/gdt.h>

/* Global descriptor table */
static GDT_TABLE g_gdt[NUM_CPUS] = {0};
static size_t num_gdt = 0;

/**
 * @brief Helper for making entry in the GDT.
 *
 * @param entry ENTRY struct to modify
 * @param base Base in memory
 * @param limit High end limit in memory
 * @param access Read/write
 * @param flags PL, Granularity, etc.
 */
void gdt_init_entry(GDT_ENTRY *entry, uint64_t base, uint64_t limit,
                           uint8_t access, uint8_t flags) {
    /* Set limit */
    entry->limit_low = GDT_LIMIT_LOW(limit);
    entry->flags_limit_high = GDT_FLAGS_LIMIT_HIGH(limit, flags);

    /* Set base */
    entry->base_low = GDT_BASE_LOW(base);
    entry->base_middle = GDT_BASE_MID(base);
    entry->base_high = GDT_BASE_HIGH(base);

    /* Set access flag */
    entry->access = access;
}

/**
 * @brief Initialization function for the GDT
 *
 */
void gdt_init(/* CPU *cpu_info */) {
    /* TODO: Must be done for each CPU */
    if (num_gdt + 1 > NUM_CPUS) {
        kloge("Trying to initialize a GDT for non-existance CPU!\n");
        halt();
    }
    GDT_TABLE *gdt = (GDT_TABLE *) &g_gdt[num_gdt++];
    memset(gdt, 0, sizeof(GDT_TABLE));

    /* NULL descriptor */
    gdt_init_entry(&(gdt->null_desc), 0, 0, 0, 0);

    /* Kernel code 16-bit */
    gdt_init_entry(&(gdt->kernel_code_16_bit), 0, 0xFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_EXECUTABLE |
        GDT_ACCESS_CODE_SEGMENT | GDT_ACCESS_CODE_READABLE,
        GDT_FLAG_16BIT | GDT_FLAG_GRANULARITY_1B);

    /* Kernel data 16-bit */
    gdt_init_entry(&(gdt->kernel_data_16_bit), 0, 0xFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 |
        GDT_ACCESS_DATA_SEGMENT | GDT_ACCESS_DATA_WRITEABLE,
        GDT_FLAG_16BIT | GDT_FLAG_GRANULARITY_1B);

    /* Kernel code 32-bit */
    gdt_init_entry(&(gdt->kernel_code_32_bit), 0, 0xFFFFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_EXECUTABLE |
        GDT_ACCESS_CODE_SEGMENT | GDT_ACCESS_CODE_READABLE,
        GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K);

    /* Kernel data 32-bit */
    gdt_init_entry(&(gdt->kernel_data_32_bit), 0, 0xFFFFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 |
        GDT_ACCESS_DATA_SEGMENT | GDT_ACCESS_DATA_WRITEABLE,
        GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K);

    /* Kernel code 64-bit */
    gdt_init_entry(&(gdt->kernel_code_64_bit), 0, 0xFFFFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_EXECUTABLE |
        GDT_ACCESS_CODE_SEGMENT | GDT_ACCESS_CODE_READABLE,
        GDT_FLAG_64BIT | GDT_FLAG_GRANULARITY_4K);

    /* Kernel data 64-bit */
    gdt_init_entry(&(gdt->kernel_data_64_bit), 0, 0xFFFFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 |
        GDT_ACCESS_DATA_SEGMENT | GDT_ACCESS_DATA_WRITEABLE,
        GDT_FLAG_64BIT | GDT_FLAG_GRANULARITY_4K);

    /* User code 64-bit */
    gdt_init_entry(&(gdt->user_code_64_bit), 0, 0xFFFFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_EXECUTABLE |
        GDT_ACCESS_CODE_SEGMENT | GDT_ACCESS_CODE_READABLE,
        GDT_FLAG_64BIT | GDT_FLAG_GRANULARITY_4K);

    /* User data 64-bit */
    gdt_init_entry(&(gdt->user_data_64_bit), 0, 0xFFFFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 |
        GDT_ACCESS_DATA_SEGMENT | GDT_ACCESS_DATA_WRITEABLE,
        GDT_FLAG_64BIT | GDT_FLAG_GRANULARITY_4K);

    GDT_DESCRIPTOR g = {
        .size = sizeof(GDT_TABLE) - 1,
        .offset = (uint64_t) gdt
    };

    gdt_load(&g);
    klogi("Finished GDT initialization for CPU: %d at %x\n", num_gdt - 1, gdt);
}

/* TODO: Complete for each CPU */
void gdt_init_tss(/* CPU *cpu_info */) {
    // GDT_DESCRIPTOR gdtr;
    // __asm__ volatile("sgdt %0"
    //                  :
    //                  : "m"(gdtr)
    //                  : "memory");
    // GDT_TABLE *gt = (GDT_TABLE *) (gdtr.offset);
}
