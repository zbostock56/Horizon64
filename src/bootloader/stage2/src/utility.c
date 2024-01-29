#include "../include/utility.h"
#include "../include/stdio.h"

#if DEBUG
void print_partition_disk(PARTITION *part) {
    printf("---- Printing Partition's Disk Attribute ----\n");
    printf("DISK->id: %d\n", part->disk->id);
    printf("DISK->cylinders: %d\n", part->disk->cylinders);
    printf("DISK->sectors: %d\n", part->disk->sectors);
    printf("DISK->heads: %d\n", part->disk->heads);
    printf("---- Printing Partiton ----\n");
    printf("PARTITION->partition_offset: %d\n", part->partition_offset);
    printf("PARTITION->partition_size: %d\n", part->partition_size);
}
#endif

void hang_system(const char *str) {
    if (str) {
        printf("%s\n");
    }
    printf("Hanging system\n");
    for (;;);
}