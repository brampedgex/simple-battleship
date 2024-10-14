#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"

int main(int argc, const char** argv) {
    printf("Welcome to battleship! Please enter your name: ");

    char* line = NULL;
    size_t size;
    if (getline(&line, &size, stdin) < 0) {
        printf("Failed to read the line!\n");
        free(line);
        return 1;
    }

    line[strcspn(line, "\n")] = 0;
    printf("Hello, %s!\n", line);
    free(line);

    struct our_board our_board;
    ourboard_init(&our_board);

    ourboard_print(&our_board);

    return 0;
}