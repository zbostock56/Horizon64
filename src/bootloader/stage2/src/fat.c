#include "../include/fat.h"
#include "../include/stdio.h"
#include "../include/memdefs.h"
#include "../include/string.h"
#include "../include/memory.h"
#include "../include/ctype.h"
#include "../include/utility.h"
#include <stddef.h>

static FAT_DATA *g_data;
static uint32_t g_data_section_lba;
static uint8_t g_fat_type;
static uint32_t g_total_sectors;
static uint32_t g_sectors_per_fat;

bool FAT_read_boot_sector(PARTITION* disk) {
    return partition_read_sectors(disk, 0, 1, g_data->BS.boot_sector_bytes);
}

bool FAT_read_fat(PARTITION *disk, size_t lba_index) {
    return partition_read_sectors(disk, g_data->BS.boot_sector.reserved_sectors + lba_index,
                            FAT_CACHE_SIZE, g_data->fat_cache);
}

void FAT_detect(PARTITION *disk) {
    uint32_t data_clusters = (g_total_sectors - g_data_section_lba) /
                             g_data->BS.boot_sector.sectors_per_cluster;
    if (data_clusters < 0xFF5) {
        g_fat_type = 12;
    } else if (g_data->BS.boot_sector.sectors_per_fat != 0) {
        g_fat_type = 16;
    } else {
        g_fat_type = 32;
    }
}

bool FAT_init(PARTITION *disk) {
    g_data = (FAT_DATA *) MEMORY_FAT_ADDR;
    
    /* Read boot sector */
    if (!FAT_read_boot_sector(disk)) {
        printf("FAT: read boot sector failed\r\n");
        return false;
    }

    /* Read file allocation table (FAT) */
    g_data->fat_cache_position = 0xFFFFFFFF;
    g_total_sectors = g_data->BS.boot_sector.total_sectors;
    if (!g_total_sectors) {
        /* FAT32 */
        g_total_sectors = g_data->BS.boot_sector.large_sector_count;
    }

    bool isFAT32 = false;
    g_sectors_per_fat = g_data->BS.boot_sector.sectors_per_fat;
    if (!g_sectors_per_fat) {
        /* FAT32 */
        isFAT32 = true;
        g_sectors_per_fat = g_data->BS.boot_sector.EBR_FAT32.sectors_per_fat;
    }

    /* Open root directory of file */
    uint32_t root_dir_lba;
    uint32_t root_dir_size;
    if (isFAT32) {
        g_data_section_lba = g_data->BS.boot_sector.reserved_sectors + 
                             g_sectors_per_fat * 
                             g_data->BS.boot_sector.fat_count;
        root_dir_lba = FAT_cluster_to_lba(g_data->BS.boot_sector.EBR_FAT32.root_directory_cluster);
        root_dir_size = 0;
    } else {
        root_dir_lba = g_data->BS.boot_sector.reserved_sectors +
                       g_sectors_per_fat *
                       g_data->BS.boot_sector.fat_count;
        root_dir_size = sizeof(FAT_DIRECTORY_ENTRY) * g_data->BS.boot_sector.dir_entry_count;
        uint32_t root_dir_sectors = (root_dir_size + g_data->BS.boot_sector.bytes_per_sector - 1) /
                                    g_data->BS.boot_sector.bytes_per_sector;
        g_data_section_lba = root_dir_lba + root_dir_sectors;
    }


    g_data->root_directory.public.handle = ROOT_DIRECTORY_HANDLE;
    g_data->root_directory.public.is_directory = true;
    g_data->root_directory.public.position = 0;
    g_data->root_directory.public.size = sizeof(FAT_DIRECTORY_ENTRY) * 
                                         g_data->BS.boot_sector.dir_entry_count;
    g_data->root_directory.opened = true;
    g_data->root_directory.first_cluster = root_dir_lba;
    g_data->root_directory.current_cluster = root_dir_lba;
    g_data->root_directory.current_sector_in_cluster = 0;

    if (!partition_read_sectors(disk, root_dir_lba, 1, g_data->root_directory.buffer)) {
        printf("FAT: read root directory failed\r\n");
        return false;
    }

    /* Calculate data section */
    FAT_detect(disk);

    /* Reset opened file descriptors */
    for (int i = 0; i < MAX_FILE_HANDLES; i++) {
        g_data->opened_files[i].opened = false;
    }
    return true;
}

uint32_t FAT_cluster_to_lba(uint32_t cluster) {
    return g_data_section_lba + (cluster - 2) * 
           g_data->BS.boot_sector.sectors_per_cluster;
}

FAT_FILE *FAT_open_entry(PARTITION *disk, FAT_DIRECTORY_ENTRY *entry) {
    /* Find empty handle */
    int handle = -1;
    for (int i = 0; i < MAX_FILE_HANDLES && handle < 0; i++) {
        if (!g_data->opened_files[i].opened) {
            handle = i;
        }
    }

    /* Out of handles */
    if (handle < 0) {
        printf("FAT: out of file handles\r\n");
        return false;
    }

    /* Setup descriptor before returning to user */
    FAT_FILE_DATA *fd = &g_data->opened_files[handle];
    fd->public.handle = handle;
    fd->public.is_directory = (entry->attributes & FAT_ATTRIBUTE_DIRECTORY) != 0;
    fd->public.position = 0;
    fd->public.size = entry->size;
    fd->first_cluster = entry->first_cluster_low + 
                        ((uint32_t) entry->first_cluster_high << 16);
    fd->current_cluster = fd->first_cluster;
    fd->current_sector_in_cluster = 0;

    if (!partition_read_sectors(disk, FAT_cluster_to_lba(fd->current_cluster), 1, fd->buffer)) {
        printf("FAT: open entry failed - read error cluster=%u lba=%u\n", 
               fd->current_cluster, FAT_cluster_to_lba(fd->current_cluster));
        for (int i = 0; i < 11; i++) {
            printf("%c", entry->name[i]);
        }
        printf("\n");
        return false;
    }

    fd->opened = true;
    return &fd->public;
}

uint32_t FAT_next_cluster(PARTITION *disk, uint32_t current_cluster) {    
    uint32_t fat_index;
    if (g_fat_type == 12) {
        fat_index = current_cluster * 3 / 2;
    } else if (g_fat_type == 16) {
        fat_index = current_cluster * 2;
    } else {
        fat_index = current_cluster * 4;
    }

    /* Make sure cache has right number */
    uint32_t fat_index_sector = fat_index / SECTOR_SIZE;
    if (fat_index_sector < g_data->fat_cache_position || 
        fat_index_sector >= g_data->fat_cache_position + FAT_CACHE_SIZE) {
        FAT_read_fat(disk, fat_index_sector);
        g_data->fat_cache_position = fat_index_sector;
    }

    fat_index -= (g_data->fat_cache_position * SECTOR_SIZE);

    uint32_t next_cluster;
    if (g_fat_type == 12) {
        if (current_cluster % 2 == 0) {
            next_cluster = (*(uint16_t *) (g_data->fat_cache + fat_index)) & 0x0FFF;
        } else {
            next_cluster = (*(uint16_t *) (g_data->fat_cache + fat_index)) >> 4;
        }

        if (next_cluster >= 0xFF8) {
            next_cluster |= 0xFFFFF000;
        }
    } else if (g_fat_type == 16) {
        next_cluster = *(uint16_t *) (g_data->fat_cache + fat_index);
        if (next_cluster >= 0x0FFF8) {
            next_cluster |= 0xFFFF0000;
        }
    } else {
        next_cluster = *(uint32_t *) (g_data->fat_cache + fat_index);
    }
    return next_cluster;
}

uint32_t FAT_read(PARTITION *disk, FAT_FILE *file, uint32_t byteCount, void *dataOut) {
    /* Get file data */
    FAT_FILE_DATA *fd = (file->handle == ROOT_DIRECTORY_HANDLE) 
        ? &g_data->root_directory 
        : &g_data->opened_files[file->handle];

    uint8_t *u8DataOut = (uint8_t *) dataOut;

    /* Don't read past the end of the file */
    if (!fd->public.is_directory || (fd->public.is_directory && fd->public.size != 0)) {
        byteCount = min(byteCount, fd->public.size - fd->public.position);
    }

    while (byteCount > 0) {
        uint32_t leftInBuffer = SECTOR_SIZE - (fd->public.position % SECTOR_SIZE);
        uint32_t take = min(byteCount, leftInBuffer);

        memcpy(u8DataOut, fd->buffer + fd->public.position % SECTOR_SIZE, take);
        u8DataOut += take;
        fd->public.position += take;
        byteCount -= take;

        /* Check if there is more data to read */
        if (leftInBuffer == take) {
            /* Special handling for root directory */
            if (fd->public.handle == ROOT_DIRECTORY_HANDLE) {
                ++fd->current_cluster;
                /* Read next sector */
                if (!partition_read_sectors(disk, fd->current_cluster, 1, fd->buffer)) {
                    printf("FAT: read error!\r\n");
                    break;
                }
            } else {
                /*  Calculate next cluster and sector to read */
                if (++fd->current_sector_in_cluster >= g_data->BS.boot_sector.sectors_per_cluster) {
                    fd->current_sector_in_cluster = 0;
                    fd->current_cluster = FAT_next_cluster(disk, fd->current_cluster);
                }

                if (fd->current_cluster >= 0xFFFFFFF8) {
                    /* Mark end of file */
                    fd->public.size = fd->public.position;
                    break;
                }
                /* Read next sector */
                if (!partition_read_sectors(disk, FAT_cluster_to_lba(fd->current_cluster) + 
                                       fd->current_sector_in_cluster, 1, fd->buffer)) {
                    printf("FAT: read error!\r\n");
                    break;
                }
            }
        }
    }
    return u8DataOut - (uint8_t *) dataOut;
}

bool FAT_read_entry(PARTITION *disk, FAT_FILE *file, FAT_DIRECTORY_ENTRY *dirEntry) {   
    return FAT_read(disk, file, sizeof(FAT_DIRECTORY_ENTRY), dirEntry) == sizeof(FAT_DIRECTORY_ENTRY);
}

void FAT_close(FAT_FILE *file) {
    if (file->handle == ROOT_DIRECTORY_HANDLE) {
        file->position = 0;
        g_data->root_directory.current_cluster = g_data->root_directory.first_cluster;
    } else {
        g_data->opened_files[file->handle].opened = false;
    }
}

bool FAT_find_file(PARTITION *disk, FAT_FILE *file, const char *name, FAT_DIRECTORY_ENTRY *entryOut) {
    char fatName[12];
    FAT_DIRECTORY_ENTRY entry;

    /* Convert from normal name to FAT style name */
    memset(fatName, ' ', sizeof(fatName));
    fatName[11] = '\0';

    /* TODO: Account for files that start */
    /* with '.', i.e. .bashrc             */
    const char *ext = strchr(name, '.');
    if (ext == NULL) {
        ext = name + 11;
    }

    for (int i = 0; i < 8 && name[i] && name + i < ext; i++) {
        fatName[i] = toupper(name[i]);
    }

    if (ext != name + 11) {
        for (int i = 0; i < 3 && ext[i + 1]; i++)
            fatName[i + 8] = toupper(ext[i + 1]);
    }

    while (FAT_read_entry(disk, file, &entry)) {
        if (memcmp(fatName, entry.name, 11) == 0) {
            *entryOut = entry;
            return true;
        }        
    }
    
    return false;
}

FAT_FILE *FAT_open(PARTITION *disk, const char *path) {
    char name[MAX_PATH_SIZE];

    // ignore leading slash
    if (path[0] == '/') {
        path++;
    }

    FAT_FILE *current = &g_data->root_directory.public;

    while (*path) {
        /* Extract file name from path */
        bool isLast = false;
        const char *delim = strchr(path, '/');
        if (delim != NULL) {
            memcpy(name, path, delim - path);
            name[delim - path] = '\0';
            path = delim + 1;
        } else {
            unsigned len = strlen(path);
            memcpy(name, path, len);
            name[len + 1] = '\0';
            path += len;
            isLast = true;
        }
        
        /* Find directory entry in current directory */
        FAT_DIRECTORY_ENTRY entry;
        if (FAT_find_file(disk, current, name, &entry)) {
            FAT_close(current);
            /* Check if directory */
            if (!isLast && entry.attributes & FAT_ATTRIBUTE_DIRECTORY == 0) {
                printf("FAT: %s not a directory\r\n", name);
                return NULL;
            }
            /* Open new directory entry */
            current = FAT_open_entry(disk, &entry);
        } else {
            FAT_close(current);
            printf("FAT: %s not found\r\n", name);
            return NULL;
        }
    }
    return current;
}
