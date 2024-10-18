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

struct placed_ship {
    int row, col;
    int dir; 
    int size;
    // How many squares are left. 0 means the ship sank.
    int count;
};

struct our_board {
    enum hit_state hits[BOARD_SIZE][BOARD_SIZE];
    enum ship ships[BOARD_SIZE][BOARD_SIZE];
    struct placed_ship placements[SHIP_COUNT];
    int ship_count;
};

struct their_board {
    enum hit_state hits[BOARD_SIZE][BOARD_SIZE];
};

void ourboard_init(struct our_board* board);
void their_board_init(struct their_board* board);

void ourboard_print(struct our_board* board);
void their_board_print(struct their_board* board);

#endif