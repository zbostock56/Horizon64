/**
 * @file echo.c
 * @author Zack Bostock
 * @brief Echoes back the input from the terminal
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <stddef.h>

#include <libc/string.h>
#include <libc/sys.h>
#include <libc/stdio.h>

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        write(STDOUT, argv[i], strlen(argv[i]));
        if (i + 1 < argc) {
            write(STDOUT, " ", 1);
        } else {
            write(STDOUT, "\n", 1);
        }
    }

    return 0;
}