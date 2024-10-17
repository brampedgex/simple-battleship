#include "util.h"
#include <stdio.h>
#include <stdlib.h>

void skipline() {
    char* line = NULL;
    size_t size;

    getline(&line, &size, stdin);
    free(line);
}

int getcharline() {
    int chr = getchar();
    skipline();
    return chr;
}