/**
 * @file clear.c
 * @author Zack Bostock
 * @brief Clears the terminal screen
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <sys.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    write(STDOUT, "\033[2J\033[H", 7);
    return 0;
}