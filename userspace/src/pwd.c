/**
 * @file pwd.c
 * @author Zack Bostock
 * @brief Print working directory
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <libc/stdio.h>
#include <libc/sys.h>

#define PATH_SIZE (256)

int main(int argc, char *argv[]) {
    char path[PATH_SIZE + 1] = {0};
    int ret = getcwd(path, PATH_SIZE);
    if (ret < 0) {
        perror("getcwd");
        fprintf(STDERR, "pwd: failed to get current working directory\n");
    } else {
        printf("%s\n", path);
    }

    return 0;
}