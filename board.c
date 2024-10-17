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

const char* ship_name(enum ship ship) {
    switch (ship) {
    case AIRCRAFT_CARRIER:
        return "Aircraft Carrier";
    case BATTLESHIP:
        return "Battleship";
    case CRUISER:
        return "Cruiser";
    case SUBMARINE:
        return "Submarine";
    case DESTROYER:
        return "Destroyer";
    default:
        return "<invalid>";
    }
}

void ourboard_init(struct our_board *board) {
    memset(board, 0, sizeof(struct our_board));
}

void their_board_init(struct their_board* board) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            board->hits[i][j] = HS_NONE;
        }
    }
}

static char ourboard_char(struct our_board* board, int r, int c) {
    char chr;

    switch (board->ships[r][c]) {
    case SHIP_NONE:
        return hit_char(board->hits[r][c]);
    case AIRCRAFT_CARRIER:
        chr = 'A';
        break;
    case BATTLESHIP:
        chr = 'B';
        break;
    case CRUISER:
        chr = 'C';
        break;
    case SUBMARINE:
        chr = 'S';
        break;
    case DESTROYER:
        chr = 'D';
        break;
    default:
        chr = ' ';
        break;
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

void their_board_print(struct their_board* board) {
    printf("    A B C D E F G H I J\n");
    
    for (int i = 0; i < BOARD_SIZE; i++) {
        printf("%2i [", i + 1);
        for (int j = 0; j < BOARD_SIZE; j++) {
            putchar(hit_char(board->hits[i][j]));
            putchar(j == BOARD_SIZE - 1 ? ']' : '|');
        }
        putchar('\n');
    }
}