#include "player.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Read a board coordinate from stdin. Returns 1 if failed, 0 if succeeded.
int player_get_coord(int* r, int* c) {
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
    return -1;
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

static void place_ship(struct our_board* board, enum ship ship, int r, int c, int dir, int size) {
    for (int i = 0; i < size; i++) {
        assert(board->ships[r][c] == SHIP_NONE);

        board->ships[r][c] = ship;

        if (dir)
            r++;
        else
            c++;

        board->ship_counts[ship]++;
    }
}

static void player_place_ship(struct our_board* board, enum ship ship, int size) {
    ourboard_print(board);

    int r, c, dir;

    while (1) {
        printf("Enter the coordinates of the top/leftmost part of the ship followed by the direction (eg. \"A1 H\"):\n");

        char* line = NULL;
        size_t size = 0;

        if (getline(&line, &size, stdin) < 0) {
            fprintf(stderr, "Failed to read a line, trying again...\n");
            free(line);
            continue;
        }

        char col_char;
        int row_num;
        char dir_char;

        if (sscanf(line, "%c%i %c", &col_char, &row_num, &dir_char) != 3) {
            printf("Invalid location.\n");
            free(line);
            continue;
        }

        free(line);

        if (row_num < 1 || row_num > 10 || col_char < 'A' || col_char > 'J' || (dir_char != 'H' && dir_char != 'V')) {
            printf("Invalid location.\n");
            continue;
        }

        r = row_num - 1;
        c = col_char - 'A';
        dir = (dir_char == 'H' ? 0 : 1);

        if (ship_obstructed(board, r, c, dir, 0)) {
            printf("This location is obstructed.\n");
            continue;
        }

        break;
    }
    
    place_ship(board, ship, r, c, dir, size);
}

static void place_ship_random(struct our_board* board, enum ship ship, int size) {
    int obstructed_table[BOARD_SIZE][BOARD_SIZE][2];
    int unobstructed_count = 0;

    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            for (int k = 0; k < 2; k++) {
                int obstructed = ship_obstructed(board, i, j, k, size);
                obstructed_table[i][j][k] = obstructed;

                if (!obstructed) {
                    unobstructed_count++;
                }
            }
        }
    }

    int idx = rand() % unobstructed_count;
    int counter = 0;

    int r = 0, c = 0, dir = 0;
    int placed = 0;

    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            for (int k = 0; k < 2; k++) {
                if (!obstructed_table[i][j][k] && idx == counter++) {
                    placed = 1;

                    r = i;
                    c = j;
                    dir = k;
                }
            }
        }
    }

    assert(placed);
    place_ship(board, ship, r, c, dir, size);
}

static void board_init_random(struct our_board* board) {
    ourboard_init(board);
    place_ship_random(board, AIRCRAFT_CARRIER, 5);
    place_ship_random(board, BATTLESHIP, 4);
    place_ship_random(board, CRUISER, 3);
    place_ship_random(board, SUBMARINE, 3);
    place_ship_random(board, DESTROYER, 2);
}

void player_create_board(struct our_board *board) {
    board_init_random(board);
    ourboard_print(board);
    printf("Your ships have been arranged randomly. Is this okay? (y/n) ");

    while (tolower(getchar()) == 'n') {
        ourboard_init(board);
        player_place_ship(board, AIRCRAFT_CARRIER, 5);
        player_place_ship(board, BATTLESHIP, 4);
        player_place_ship(board, CRUISER, 3);
        player_place_ship(board, SUBMARINE, 3);
        player_place_ship(board, DESTROYER, 2);


        ourboard_print(board);
        printf("Is this okay? (y/n) ");
    }
}