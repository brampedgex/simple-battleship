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

const char* ship_name(enum ship ship);

struct our_board {
    enum hit_state hits[BOARD_SIZE][BOARD_SIZE];
    enum ship ships[BOARD_SIZE][BOARD_SIZE];
    // Stores how many cells contain a ship of each ship type. If a number goes to zero it means the ship sank.
    int ship_counts[SHIP_COUNT];
};

struct their_board {
    enum hit_state hits[BOARD_SIZE][BOARD_SIZE];
};

void ourboard_init(struct our_board* board);
void their_board_init(struct their_board* board);

void ourboard_print(struct our_board* board);
void their_board_print(struct their_board* board);

#endif