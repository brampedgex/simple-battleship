#include "board.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

char hit_char(enum hit_state hit) {
    if (hit == 0)
        return ' ';

    switch (hit) {
    case HS_NONE:
        return ' ';
    case HIT:
        return 'x';
    case MISS:
        return 'o';
    }
}

void ourboard_init(struct our_board *board) {
    memset(board, 0, sizeof(struct our_board));
}

static char ourboard_char(struct our_board* board, int r, int c) {
    char chr;

    switch (board->ships[r][c]) {
    case SHIP_NONE:
        return hit_char(board->hits[r][c]);
    case AIRCRAFT_CARRIER:
        chr = 'A';
    case BATTLESHIP:
        chr = 'B';
    case CRUISER:
        chr = 'C';
    case SUBMARINE:
        chr = 'S';
    case DESTROYER:
        chr = 'D';
    default:
        chr = ' ';
    }

    if (board->hits[r][c] == HIT)
        chr = tolower(chr);

    return chr;
}

void ourboard_print(struct our_board *board) {
    printf("    A B C D E F G H I J\n");
    
    for (int i = 0; i < BOARD_SIZE; i++) {
        printf("%2i [", i + 1);
        for (int j = 0; j < BOARD_SIZE; j++) {
            putchar(ourboard_char(board, i, j));
            putchar(j == BOARD_SIZE - 1 ? ']' : '|');
        }
        putchar('\n');
    }
}