/**
 * @file mkdir.c
 * @author Zack Bostock
 * @brief Make a directory
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <libc/stdio.h>
#include <libc/sys.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(STDERR, "Usage: mkdir directory_name ...");
        exit(1);
    }

    for (int i = 1; i < argc; i++) {
        if (mkdirat(argv[i]) < 0) {
            perror("mkdirat");
            fprintf(STDERR, "mkdir: directory '%s' failed to be created\n", argv[i]);
            exit(1);
        }
    }
    return 0;
}