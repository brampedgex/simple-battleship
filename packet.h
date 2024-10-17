#ifndef _PACKET_H
#define _PACKET_H

#include "board.h"
#include "util.h"
#include <stdint.h>

enum packet_type {
    PKT_CLIENT_HELLO,   // Sent from the client with a magic value
    PKT_SERVER_HELLO,   // Sent fromfrom the server to the client with a magic value

    PKT_SERVER_READY,   // Server is ready
    
    PKT_SHIPS_READY,    // A player's ships have been placed
    
    PKT_BEGIN_GAME,     // Sent from the server to indicate the game has begun.
    
    PKT_MOVE,           // Player has made a move
    PKT_MOVE_RESULT,    // Result of a move (hit, miss, ship sunk)
    
    PKT_END_GAME,       // Sent from the server to end a game, including the winner.

    PKT_DISCONNECT,     // Sent when an error is encountered.
};

enum peer_type {
    PEER_CLIENT,
    PEER_SERVER
};

enum net_move_result {
    NET_HIT,
    NET_MISS,
    NET_SINK
};

struct packet_header {
    enum packet_type type;
    u16 length;
};

// weak attempt at writing BATTLE in hex
#define NET_MAGIC 0x00BA117E

struct pkt_begin_game {
    // Who will go first.
    enum peer_type first;  
};

struct pkt_move {
    int row, col;
};

struct pkt_move_result {
    enum net_move_result result;
    // Sink data. this only filled in if a ship was sunk
    enum ship ship_type;
    int ship_row, ship_col, ship_dir;
};

struct pkt_end_game {
    enum peer_type winner;
};

struct pkt_disconnect {
    char reason[512];
};

struct packet {
    enum packet_type type;
    union {
        struct pkt_begin_game begin_game;
        struct pkt_move move;
        struct pkt_move_result move_result;
        struct pkt_end_game end_game;
        struct pkt_disconnect disconnect;
    };
};

#endif