/**
 * @file init.c
 * @author Zack Bostock
 * @brief Main initialization function for userspace
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <string.h>
#include <sys.h>
#include <stdio.h>

static char *argv[] = {
    "hsh",
    NULL
};

int main() {
    int pid;
    // printf(
    // "                     _                     __    _  _   \n"
    // "  /\\  /\\ ___   _ __ (_) ____ ___   _ __   / /_  | || |  \n"
    // " / /_/ // _ \\ | '__|| ||_  // _ \\ | '_ \\ | '_ \\ | || |_ \n"
    // "/ __  /| (_) || |   | | / /| (_) || | | || (_) ||__   _|\n"
    // "\\/ /_/  \\___/ |_|   |_|/___|\\___/ |_| |_| \\___/    |_|  \n");

    /* Loop to start shell program */
    for (;;) {
        // printf("init: starting shell...\n");
        pid = fork();
        if (pid < 0) {
            /* Failure */
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            /* Child process */
            execv("/bin/hsh", argv);
            printf("init: execution of shell failed\n");
            exit(1);
        } else {
            /* Parent process */
            if (wait(-1) < 0) {
                perror("waitpid");
                exit(1);
            }
        }
    }

    return 0;
}
