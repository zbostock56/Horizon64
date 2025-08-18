/**
 * @file nf.c
 * @author Zack Bostock
 * @brief Create a new file
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <libc/sys.h>
#include <libc/stdio.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(STDERR, "Usage: nf filename ...");
        exit(1);
    }

    for (int i = 1; i < argc; i++) {
        int fd = open(argv[i], O_CREAT | O_RDWR);
        if (fd < 0) {
            fprintf(STDERR, "nf: Failed to open '%s'\n", argv[i]);
            perror("open");
            exit(1);
        }
        if (close(fd) < 0) {
            fprintf(STDERR, "nf: Failed to close '%s'\n", argv[i]);
            perror("close");
            exit(1);
        }
    }
    return 0;
}