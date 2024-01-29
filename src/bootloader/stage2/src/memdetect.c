#include "../include/memdetect.h"
#include "../include/stdio.h"

/* Note: As long as kernel does not overwrite the memory */
/* of the stage2 bootloader, this structure should stay  */
/* accessable even once the kernel takes over.           */
MEMORY_REGION g_mem_regions[MAX_REGIONS];
int g_mem_regions_count;

void print_memory_region(E820_MEMORY_BLOCK block) {
    if (block.type == E820_USABLE) {
        printf("memdetect: base=0x%llx | length=0x%llx | type=USABLE\n",
        block.base, block.length);
    } else if (block.type == E820_RESERVED) {
        printf("memdetect: base=0x%llx | length=0x%llx | type=RESERVED\n",
        block.base, block.length);
    } else if (block.type == E820_ACPI_RECLAIMABLE) {
        printf("memdetect: base=0x%llx | length=0x%llx | type=ACPI_RECLAIMABLE\n",
        block.base, block.length);
    } else if (block.type == E820_ACPI_NVS) {
        printf("memdetect: base=0x%llx | length=0x%llx | type=ACPI_NVS\n",
        block.base, block.length);
    } else {
        printf("memdetect: base=0x%llx | length=0x%llx | type=BAD_MEMORY\n",
        block.base, block.length);
    }
}

void memory_detect(MEMORY_INFO *mem) {
    E820_MEMORY_BLOCK block;
    uint32_t continuation = 0;
    g_mem_regions_count = 0;

    /* BIOS interrupt will return 0 and/or set continuation */
    /* to 0 if it is finished reading all mem regions       */

    int bios_ret = x86_E820_get_next_block(&block, &continuation);

    while (bios_ret > 0 && continuation != 0) {

        g_mem_regions[g_mem_regions_count].base = block.base;
        g_mem_regions[g_mem_regions_count].length = block.length;
        g_mem_regions[g_mem_regions_count].type = block.type;
        g_mem_regions[g_mem_regions_count].acpi = block.acpi;
        g_mem_regions_count++;

        /* Print to the screen during boot procedure */
        print_memory_region(block);

        bios_ret = x86_E820_get_next_block(&block, &continuation);
    }
    mem->region_count = g_mem_regions_count;
    mem->regions = g_mem_regions;
}