#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, const char** argv) {
    printf("Welcome to battleship! First enter your name: ");

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
    return 0;
}