#ifndef _PACKET_H
#define PACKET_H

enum packet_type {
    PKT_HELLO,          // Sent from the client with a magic value
    PKT_START,          // Sent from the server, ready to start a game
    PKT_PLAYER_READY,   // Player's ships have been placed
    PKT_BEGIN_GAME,     // Sent from the server to indicate the game has begun.
    PKT_MOVE,           // Player has made a move
    PKT_MOVE_RESULT,    // Result of a move (hit, miss, ship sunk)
    PKT_END_GAME,       // Sent from the server to end a game, including the winner.
};

struct packet_header {
    enum packet_type type;
    unsigned length;
};

#endif