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

static int  flag_all   = 0;    // -a
static int  flag_long  = 0;    // -l

/**
 * @brief Permission bits to rwxrwxrwx string
 *
 * @param m Mode
 * @param out Output buffer
 */
static void mode_to_str(MODE m, char *out) {
    const char *rwx = "rwx";
    for (int i = 0; i < 9; i++) {
        out[i] = (m & (1 << (8 - i))) ? rwx[i % 3] : '-';
    }
    out[9] = '\0';
}

/**
 * @brief Helper to get the path name formatted correctly
 *
 * @param path Path to format
 * @param buf Output buffer
 * @return char * Buffer with formatted text
 */
static char *fmtname(char *path, char *buf) {
    char *p;
    for (p = path + strlen(path); p >= path && *p != '/'; p--);
    p++;
    if (strlen(p) >= DIRSIZE)
        return p;
    memcpy(buf, p, strlen(p));
    buf[strlen(p)] = '\0';
    return buf;
}

/**
 * @brief list directory contents
 *
 * @param path Path to list directory contents of
 */
void ls(char *path) {
    char fmtbuff[DIRSIZE + 1]   = {0};
    char fullpath[DIRSIZE + 1]  = {0};
    int  fd                     = INVALID_FD;
    DIRENT de;
    STAT   st;

    /* open target */
    if ((fd = open(path, 0)) < 0) {
        fprintf(STDERR, "ls: cannot access '%s': No such file or directory\n", path);
        return;
    }

    if (fstat(fd, &st) < 0) {
        fprintf(STDERR, "ls: cannot stat '%s'\n", path);
        close(fd);
        return;
    }

    if ((st.st_mode & S_IFMT) == S_IFDIR) {
        /* directory: loop entries */
        while (readdir(fd, &de) >= 0) {
            if (de.d_ino == 0)
                continue;
            /* skip dotfiles unless -a */
            if (!flag_all && de.d_name[0] == '.')
                continue;

            /* build path/entry */
            snprintf(fullpath, sizeof(fullpath), "%s/%s", path, de.d_name);

            if (stat(fullpath, &st) < 0) {
                fprintf(STDERR, "ls: cannot stat '%s'\n", fullpath);
                continue;
            }

            if (flag_long) {
                char perms[10];
                mode_to_str(st.st_mode, perms);
                printf("%s %4d %8d %s\n",
                       perms,
                       st.st_ino,
                       st.st_size,
                       fmtname(fullpath, fmtbuff));
            } else {
                printf("%s\n", fmtname(fullpath, fmtbuff));
            }
        }
    } else {
        /* not a directory */
        if (flag_long) {
            char perms[10];
            mode_to_str(st.st_mode, perms);
            printf("%s %4d %8d %s\n",
                   perms,
                   st.st_ino,
                   st.st_size,
                   path);
        } else {
            printf("%s\n", path);
        }
    }

    close(fd);
}

int main(int argc, char *argv[]) {
    int first_path = 1;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            first_path = i;
            break;
        }
        for (int j = 1; argv[i][j]; j++) {
            switch (argv[i][j]) {
            case 'a':
                flag_all = 1;
                break;
            case 'l':
                flag_long = 1;
                break;
            default:
                fprintf(STDERR, "ls: invalid option -- '%c'\n", argv[i][j]);
                return 1;
            }
        }
    }

    /* if no paths given, default to “.” */
    if (first_path == 1 && argc == 1) {
        ls(".");
    } else {
        for (int i = first_path; i < argc; i++) {
            ls(argv[i]);
        }
    }
    return 0;
}