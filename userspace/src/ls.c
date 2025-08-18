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
#define MAX_PATH_LEN (4096)

static int  flag_all   = 0;    // -a
static int  flag_long  = 0;    // -l

/**
 * @brief Permission bits to rwxrwxrwx string with file type indicator
 *
 * @param m Mode
 * @param out Output buffer (must be at least 11 bytes)
 */
static void mode_to_str(MODE m, char *out) {
    const char *rwx = "rwx";

    /* First character indicates file type */
    if ((m & S_IFMT) == S_IFDIR) {
        out[0] = 'd';
    } else if ((m & S_IFMT) == S_IFLNK) {
        out[0] = 'l';
    } else if ((m & S_IFMT) == S_IFBLK) {
        out[0] = 'b';
    } else if ((m & S_IFMT) == S_IFCHR) {
        out[0] = 'c';
    } else if ((m & S_IFMT) == S_IFIFO) {
        out[0] = 'p';
    } else if ((m & S_IFMT) == S_IFSOCK) {
        out[0] = 's';
    } else {
        out[0] = '-';  /* Regular file */
    }

    /* Permission bits */
    for (int i = 0; i < 9; i++) {
        out[i + 1] = (m & (1 << (8 - i))) ? rwx[i % 3] : '-';
    }
    out[10] = '\0';
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

    if (!path || !buf) {
        return path;
    }

    /* Find last '/' in path */
    for (p = path + strlen(path); p >= path && *p != '/'; p--);
    p++;

    /* If filename is too long, return original */
    if (strlen(p) >= DIRSIZE) {
        return p;
    }

    /* Copy filename to buffer */
    strcpy(buf, p);
    return buf;
}

/**
 * @brief Safely build full path from directory and filename
 *
 * @param dir_path Directory path
 * @param filename Filename to append
 * @param full_path Output buffer
 * @param max_len Maximum buffer size
 * @return int 0 on success, -1 on error
 */
static int build_path(const char *dir_path, const char *filename,
                     char *full_path, size_t max_len) {
    size_t dir_len, file_len, total_len;
    int needs_slash;

    if (!dir_path || !filename || !full_path || max_len == 0) {
        return -1;
    }

    dir_len = strlen(dir_path);
    file_len = strlen(filename);
    needs_slash = (dir_len > 0 && dir_path[dir_len - 1] != '/');
    total_len = dir_len + (needs_slash ? 1 : 0) + file_len;

    if (total_len >= max_len) {
        return -1; /* Path too long */
    }

    /* Build path safely */
    strcpy(full_path, dir_path);
    if (needs_slash) {
        strcat(full_path, "/");
    }
    strcat(full_path, filename);

    return 0;
}

/**
 * @brief Print file information in requested format
 *
 * @param path File path
 * @param st File statistics
 * @param display_name Name to display (or NULL to use path)
 */
static void print_file_info(const char *path, const STAT *st, const char *display_name) {
    char fmtbuff[DIRSIZE + 1] = {0};
    const char *name = display_name;

    if (!name) {
        name = fmtname((char*)path, fmtbuff);
    } else {
        /* Format the display name */
        name = fmtname((char*)display_name, fmtbuff);
    }

    if (flag_long) {
        char perms[11];
        mode_to_str(st->st_mode, perms);
        printf("%s %8u %8u %s\n",
               perms,
               (unsigned int)st->st_ino,
               (unsigned int)st->st_size,
               name);
    } else {
        printf("%s\n", name);
    }
}

/**
 * @brief Read directory contents and display them
 *
 * @param path Directory path
 * @param fd File descriptor for directory
 * @return int 0 on success, -1 on error
 */
static int read_directory(const char *path, int fd) {
    DIRENT de;
    STAT st;
    char fullpath[MAX_PATH_LEN];
    int read_result;
    int entries_shown = 0;
    int error_count = 0;

    /* Read directory entries */
    while ((read_result = readdir(fd, &de)) >= 0) {
        /* Skip invalid entries */
        if (de.d_ino == 0) {
            continue;
        }

        /* Skip hidden files unless -a flag is set */
        if (!flag_all && de.d_name[0] == '.') {
            continue;
        }

        /* Build full path for this entry */
        if (build_path(path, de.d_name, fullpath, sizeof(fullpath)) < 0) {
            fprintf(STDERR, "ls: path too long: %s/%s\n", path, de.d_name);
            error_count++;
            continue;
        }

        /* Get file statistics */
        if (stat(fullpath, &st) < 0) {
            fprintf(STDERR, "ls: cannot stat '%s': stat failed\n", fullpath);
            error_count++;
            continue;
        }

        /* Display file information */
        print_file_info(fullpath, &st, de.d_name);
        entries_shown++;
    }

    /* Warn about errors but don't fail completely */
    if (error_count > 0) {
        fprintf(STDERR, "ls: %d errors occurred reading '%s'\n", error_count, path);
    }

    return 0;
}

/**
 * @brief Validate path parameter
 *
 * @param path Path to validate
 * @return int 0 if valid, -1 if invalid
 */
static int validate_path(const char *path) {
    if (!path) {
        return -1;
    }

    size_t len = strlen(path);
    if (len == 0 || len >= MAX_PATH_LEN) {
        return -1;
    }

    return 0;
}

/**
 * @brief List directory contents or file information
 *
 * @param path Path to list directory contents of
 */
void ls(char *path) {
    int fd = INVALID_FD;
    STAT st;

    /* Validate input */
    if (validate_path(path) < 0) {
        fprintf(STDERR, "ls: invalid path '%s'\n", path ? path : "(null)");
        return;
    }

    /* Open target */
    if ((fd = open(path, O_RDONLY)) < 0) {
        fprintf(STDERR, "ls: cannot access '%s': No such file or directory\n", path);
        return;
    }

    /* Get file/directory information */
    if (fstat(fd, &st) < 0) {
        fprintf(STDERR, "ls: cannot stat '%s': fstat failed\n", path);
        close(fd);
        return;
    }

    /* Handle based on file type */
    if ((st.st_mode & S_IFMT) == S_IFDIR) {
        /* Directory: read and display contents */
        if (read_directory(path, fd) < 0) {
            /* Error message already printed */
            close(fd);
            return;
        }
    } else {
        /* Regular file or other: display file info */
        print_file_info(path, &st, NULL);
    }

    /* Clean up */
    if (close(fd) < 0) {
        fprintf(STDERR, "ls: warning: error closing '%s'\n", path);
    }
}

/**
 * @brief Parse command line arguments
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @return int Index of first non-option argument, or argc if none
 */
static int parse_arguments(int argc, char *argv[]) {
    int first_path = argc; /* Default: no paths found */

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            /* Found first non-option argument */
            first_path = i;
            break;
        }

        /* Process option flags */
        for (int j = 1; argv[i][j]; j++) {
            switch (argv[i][j]) {
            case 'a':
                flag_all = 1;
                break;
            case 'l':
                flag_long = 1;
                break;
            case 'h':
                printf("Usage: ls [-al] [file...]\n");
                printf("  -a    show hidden files\n");
                printf("  -l    long format\n");
                printf("  -h    show this help\n");
                return -1; /* Signal to exit */
            default:
                fprintf(STDERR, "ls: invalid option -- '%c'\n", argv[i][j]);
                fprintf(STDERR, "Try 'ls -h' for more information.\n");
                return -2; /* Signal error */
            }
        }
    }

    return first_path;
}

/**
 * @brief Main function
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @return int Exit status
 */
int main(int argc, char *argv[]) {
    int first_path;

    /* Parse command line arguments */
    first_path = parse_arguments(argc, argv);
    if (first_path == -1) {
        return 0; /* Help was shown */
    } else if (first_path == -2) {
        return 1; /* Error occurred */
    }

    /* If no paths given, default to current directory */
    if (first_path >= argc) {
        ls(".");
    } else {
        /* Process each path argument */
        int multiple_paths = (argc - first_path > 1);

        for (int i = first_path; i < argc; i++) {
            /* Show path header for multiple paths */
            if (multiple_paths) {
                if (i > first_path) {
                    printf("\n");
                }
                printf("%s:\n", argv[i]);
            }

            ls(argv[i]);
        }
    }

    return 0;
}