/**
 * @file cat.c
 * @author Zack Bostock
 * @brief Prints out the contents of a file
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <stddef.h>
#include <stdint.h>

#include <libc/stdio.h>
#include <libc/string.h>
#include <libc/sys.h>

static char buf[512] = {0};

/**
 * @brief Outputs contents of a file descriptor
 *
 * @param fd File descriptor to read from
 */
void cat_impl(int fd) {
    int n;
    /* Continue reading while the number of bytes read is greater than 0 */
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        if (write(STDOUT, buf, n) != n) {
            fprintf(STDERR, "cat: write error\n");
            exit(1);
        }
    }
    /* If the return value is less than one, an error occured from reading */
    if (n < 0) {
        fprintf(STDERR, "cat: read error\n");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        cat_impl(STDIN);
    } else {
        int fd;
        char buf2[128] = {0};
        for (int i = 1; i < argc; i++) {
            if ((fd = open(argv[i], O_RDONLY)) < 0) {
                strcat(buf2, "cat: cannot open ");
                strcat(buf2, argv[i]);
                strcat(buf2, "\n");
                fprintf(STDERR, buf2);
                exit(1);
            }
            cat_impl(fd);
            close(fd);
        }
    }
    exit(0);
}