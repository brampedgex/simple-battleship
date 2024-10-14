#include "player.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Read a board coordinate from stdin. Returns 1 if failed, 0 if succeeded.
static int get_coord(int* r, int* c) {
    char* line = NULL;
    size_t size = 0;

    if (getline(&line, &size, stdin) < 0)
        goto invalid;

    char col;
    int row;

    if (sscanf(line, "%c%i", &col, &row) != 2)
        goto invalid;

    if (row < 1 || row > 10)
        goto invalid;

    if (col < 'A' || col > 'J')
        goto invalid;

    *r = row - 1;
    *c = col - 'A';

    free(line);
    return 0;

invalid:
    printf("Invalid coordinate.\n");
    free(line);
    return 1;
}

// Gets a direction. 0 for horizontal, 1 for vertical. Returns 2 if "cancel" was typed.
static int get_direction(int* dir) {
    char* line = NULL;
    size_t size = 0;

    if (getline(&line, &size, stdin) < 0)
        goto invalid;

    if (strcmp(line, "cancel") == 0) {
        free(line);
        return 2;
    }

    if (strlen(line) != 2)
        goto invalid;

    switch (tolower(*line)) {
    case 'h':
        *dir = 0;
        break;
    case 'v':
        *dir = 1;
        break;
    default:
        goto invalid;
    }

    free(line);
    return 0;

invalid:
    printf("Invalid direction.\n");
    free(line);
    return 1;
}

static int ship_obstructed(struct our_board* board, int r, int c, int dir, int size) {
    int cur_row = r, cur_col = c;
    
    for (int i = 0; i < size; i++) {
        if (cur_row >= BOARD_SIZE || cur_col >= BOARD_SIZE)
            return 1;

        if (board->ships[cur_row][cur_col] != SHIP_NONE)
            return 1;

        if (dir)
            cur_row++;
        else
            cur_col++;
    }
    
    return 0;
}

static void place_ship(struct our_board* board, enum ship ship, const char* name, int size) {
    ourboard_print(board);

    int r, c, dir;

cancel:
    while (1) {
        printf("Choose a square to put your %s: ", name);
        if (get_coord(&r, &c))
            continue;

        if (ship_obstructed(board, r, c, 0, size) 
            && ship_obstructed(board, r, c, 1, size)) {
            printf("There is no way to place the ship at that square.\n");
            continue;
        }

        break;
    }

    while (1) {
        printf("Choose a direction to orient the ship (H/V, or cancel to start over): ");

        int result = get_direction(&dir);

        if (result == 2)
            goto cancel;

        if (result == 1)
            continue;

        if (ship_obstructed(board, r, c, dir, size)) {
            printf("Cannot place a ship that way.\n");
            continue;
        }

        break;
    }

    for (int i = 0; i < size; i++) {
        board->ships[r][c] = ship;

        if (dir)
            r++;
        else
            c++;

        board->ship_counts[ship]++;
    }
}

void player_create_board(struct our_board *board) {
retry:
    ourboard_init(board);
    place_ship(board, AIRCRAFT_CARRIER, "Aircraft Carrier", 5);
    place_ship(board, BATTLESHIP, "Battleship", 4);
    place_ship(board, CRUISER, "Cruiser", 3);
    place_ship(board, SUBMARINE, "Submarine", 3);
    place_ship(board, DESTROYER, "Destroyer", 2);

    ourboard_print(board);
    printf("Is this okay? (y/n) ");

    if (tolower(getchar()) != 'y')
        goto retry;
}