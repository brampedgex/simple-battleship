#include "player.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Read a board coordinate from stdin. Returns 1 if failed, 0 if succeeded.
static int get_coord(int* r, int* c) {
    char* line = NULL;
    size_t size = 0;

    if (getline(&line, &size, stdin) < 0)
        goto invalid;

    char row;
    int col;

    if (sscanf(line, "%c%i", &row, &col) != 2)
        goto invalid;

    if (row < 'A' || row > 'J')
        goto invalid;

    if (col < 1 || col > 10)
        goto invalid;

    *r = row - 'A';
    *c = col - 1;

    free(line);
    return 0;

invalid:
    printf("Invalid coordinate.\n");
    free(line);
    return 1;
}

void player_create_board(struct our_board *board) {
    int r, c;
    do {
        printf("Enter a coordinate: ");
    } while (get_coord(&r, &c));

    printf("You entered: %i %i\n", r, c);
}