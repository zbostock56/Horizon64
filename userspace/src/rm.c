/**
 * @file rm.c
 * @author Zack Bostock
 * @brief Remove a file
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <stdio.h>
#include <sys.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(STDERR, "Usage: rm file ...\n");
        exit(1);
    }

    for (int i = 1; i < argc; i++) {
        if (unlink(argv[i]) < 0) {
            perror("unlink");
            fprintf(STDERR, "rm: %s failed to be removed\n", argv[i]);
            break;
        }
    }

    return 0;
}