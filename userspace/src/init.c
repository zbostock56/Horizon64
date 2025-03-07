/**
 * @file init.c
 * @author Zack Bostock
 * @brief Main initialization function for userspace
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <libc/string.h>
#include <libc/sys.h>

static char *argv[] = {
    "hsh",
    NULL
};

int main() {
    int pid;

    /* Loop to start shell program */
    for (;;) {
        printf("init: starting shell...\n");
        pid = fork();
        if (pid < 0) {
            /* Failure */
            printf("init: fork failed\n");
            exit(1);
        } else if (pid == 0) {
            /* Child process */
            execv("/bin/hsh", argv);
            printf("init: execution of shell failed\n");
            exit(1);
        } else {
            /* Parent process */
            wait(-1);
        }
    }

    return 0;
}