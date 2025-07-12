/**
 * @file ls.c
 * @author Zack Bostock
 * @brief 
 * ls - list files of a specified directory
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <stddef.h>
#include <stdint.h>

#include <libc/stdio.h>
#include <libc/string.h>
#include <libc/sys.h>

#define DIRSIZE     (1024)
#define INVALID_FD  (-1)

/**
 * @brief Helper to get the path name formatted correctly
 * 
 * @param path Path to format
 * @param buf Output bufferj
 * @return char * Buffer with formatted text
 */
char *fmtname(char *path, char *buf) {
    char *p;
    for (p = path + strlen(path); p >= path && *p != '/'; p--);
    p++;

    if (strlen(p) >= DIRSIZE)
        return p;
    memcpy(buf, p, strlen(p));
    memset(buf + strlen(p), ' ', DIRSIZE - strlen(p));
    buf[strlen(p)] = '\0';
    return buf;
}

void ls(char *path) {
    char fmtbuff[DIRSIZE + 1] = {0};
    char buf[DIRSIZE + 1] = {0};
    char *p;
    int fd = INVALID_FD; 
    DIRENT de;
    STAT st;

    if (!strncmp(path, ".", 1)) {
        if (getcwd(buf, sizeof(buf) - 1) < 0) {
            perror("ls: getcwd failed\n");
            exit(1);
        }
    }

    if ((fd = open(path, 0)) < 0) {
        fprintf(STDERR, "ls: cannot access '%s': No such file or directory\n", path);
        return;
    }

    if (fstat(fd, &st) < 0) {
        fprintf(STDERR, "ls: cannot stat '%s'\n", path);
        close(fd);
        return;
    }

    switch (st.st_mode & S_IFMT) {
        case S_IFDIR:
            strcpy(buf, path);
            p = buf + strlen(buf);
            *p++ = '/';
            while (readdir(fd, &de) >= 0) {
                if (de.d_ino == 0) {
                    continue;
                }
                memcpy(p, de.d_name, sizeof(de.d_name));
                *(p + sizeof(de.d_name)) = 0;
                if (stat(buf, &st) < 0) {
                    fprintf(STDERR, "ls: cannot stat '%s'\n", buf);
                    continue;
                }

                printf("%s\t0x%x\t%d\t%d\n", fmtname(buf, fmtbuff),
                                             (st.st_mode & S_IFMT) >> 12,
                                             st.st_ino,
                                             st.st_size);
            }
            break;
        default:
            fprintf(STDERR, "%s\n", path);
            break;
    }
    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        ls(".");
        exit(0);
    }
    for (int i = 1; i < argc; i++) {
        ls(argv[i]);
    }
    exit(0);
}