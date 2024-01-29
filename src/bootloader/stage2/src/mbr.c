#include "../include/mbr.h"
#include "../include/memory.h"
#include "../include/stdio.h"
#include "../include/utility.h"

void MBR_detect_partition(PARTITION *part, DISK *disk, void *partition) {
    part->disk = disk;
    if (disk->id < 0x80) {
        part->partition_offset = 0;
        part->partition_size = (uint32_t) (disk->cylinders) *
                               (uint32_t) (disk->heads) *
                               (uint32_t) (disk->sectors);
    } else {
        MBR_ENTRY *entry = (MBR_ENTRY *) segoffset_to_linear(partition);
        part->partition_offset = entry->lba_start;
        part->partition_size = entry->size;
    }
    #if DEBUG
    print_partition_disk(part);
    #endif
}

bool partition_read_sectors(PARTITION *part, uint32_t lba, uint8_t sectors, void *lower_dataout) {
    return disk_read_sectors(part->disk, lba + part->partition_offset, sectors, lower_dataout);
}