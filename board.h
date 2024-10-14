#ifndef _BOARD_H
#define _BOARD_H

#define BOARD_SIZE 10

enum hit_state {
    HS_NONE,
    HIT,
    MISS
};

char hit_char(enum hit_state hit);

enum ship {
    SHIP_NONE,
    AIRCRAFT_CARRIER,
    BATTLESHIP,
    CRUISER,
    SUBMARINE,
    DESTROYER,
    SHIP_COUNT
};

struct our_board {
    enum hit_state hits[BOARD_SIZE][BOARD_SIZE];
    enum ship ships[BOARD_SIZE][BOARD_SIZE];
    // Stores how many cells contain a ship of each ship type. If a number goes to zero it means the ship sank.
    int ship_counts[SHIP_COUNT];
};

void ourboard_init(struct our_board* board);

void ourboard_print(struct our_board* board);

#endif