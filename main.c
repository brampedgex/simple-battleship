#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>
#include <unistd.h>

#include "board.h"
#include "packet.h"
#include "player.h"
#include "network.h"

struct game_state {
    struct our_board board;
    struct their_board their_board;
    enum peer_type turn;
};

#define EXPECT_PACKET(conn, packet, pkttype, name) \
    if (recv_packet((conn), &(packet)))         \
        exit(1);                                \
    switch ((packet).type) {                    \
    case pkttype:                               \
        break;                                  \
    case PKT_DISCONNECT:                        \
        fprintf(stderr, "disconnected: %s\n", packet.disconnect.reason);\
        exit(1);                                \
    default:                                    \
        disconnectf(conn, "expected a " name " packet");    \
        exit(1);                                \
    }

static void play_game(struct connection* conn) {
    struct game_state state;
    struct packet incoming, outgoing;

    player_create_board(&state.board);
    their_board_init(&state.their_board);

    outgoing.type = PKT_SHIPS_READY;
    send_packet(conn, &outgoing);

    printf("Waiting for the other player...\n");

    EXPECT_PACKET(conn, incoming, PKT_SHIPS_READY, "ships ready");

    if (conn->type == PEER_SERVER) {
        outgoing.type = PKT_BEGIN_GAME;
        outgoing.begin_game.first = state.turn = (enum peer_type)(rand() % 2);
        send_packet(conn, &outgoing);
    } else {
        EXPECT_PACKET(conn, incoming, PKT_BEGIN_GAME, "begin game");
        state.turn = incoming.begin_game.first;
    }

    printf("Begin!\n");

    while (1) {
        if (state.turn == conn->type) {
            printf("\nTHEIR BOARD:\n");
            their_board_print(&state.their_board);

            int r, c;

            while (1) {
                printf("Enter the square to shoot at (eg. A1): ");
                
                if (player_get_coord(&r, &c))
                    continue;

                if (state.their_board.hits[r][c] != HS_NONE) {
                    printf("You've already shot at this square.\n");
                    continue;
                }

                break;
            }

            outgoing.type = PKT_MOVE;
            outgoing.move = (struct pkt_move){ .row = r, .col = c };
            send_packet(conn, &outgoing);

            EXPECT_PACKET(conn, incoming, PKT_MOVE_RESULT, "move result");

            switch (incoming.move_result.result) {
            case NET_HIT:
            case NET_SINK:
                state.their_board.hits[r][c] = HIT;
                break;
            case NET_MISS:
                state.their_board.hits[r][c] = MISS;
                break;
            }

            printf("\nTHEIR BOARD:\n");
            their_board_print(&state.their_board);

            switch (incoming.move_result.result) {
            case NET_HIT:
                printf("Hit! ");
                break;
            case NET_MISS:
                printf("Miss! ");
                break;
            case NET_SINK:
                printf("You sunk their %s! ", ship_name(incoming.move_result.ship_type));
                break;
            }

            if (incoming.move_result.win) {
                printf("\n\nYou won!\n");
                break;
            }
        } else {
            printf("Waiting for their move...\n");
            EXPECT_PACKET(conn, incoming, PKT_MOVE, "move");

            int r = incoming.move.row, c = incoming.move.col;

            if (state.board.hits[r][c] != HS_NONE) {
                disconnectf(conn, "attempting to hit a square that was already hit");
                exit(1);
            }

            outgoing.type = PKT_MOVE_RESULT;
            outgoing.move_result = (struct pkt_move_result){0};

            enum ship ship = state.board.ships[r][c];
            if (ship != SHIP_NONE) {
                outgoing.move_result.result = NET_HIT;
                state.board.hits[r][c] = HIT;
                if (--state.board.placements[ship].count <= 0) {
                    struct placed_ship sunk = state.board.placements[ship];
                    outgoing.move_result.result = NET_SINK;
                    outgoing.move_result.ship_row = sunk.row;
                    outgoing.move_result.ship_col = sunk.col;
                    outgoing.move_result.ship_dir = sunk.dir;
                    outgoing.move_result.ship_size = sunk.size;
                    outgoing.move_result.ship_type = ship;

                    if (--state.board.ship_count <= 0) {
                        outgoing.move_result.win = 1;
                    }
                }
            } else {
                outgoing.move_result.result = NET_MISS;
                state.board.hits[r][c] = MISS;
            }

            send_packet(conn, &outgoing);

            printf("\nYOUR BOARD:\n");
            ourboard_print(&state.board);

            switch (outgoing.move_result.result) {
            case NET_HIT:
                printf("They shot at %c%i and hit your %s!\n", 
                    c + 'A',
                    r + 1,
                    ship_name(ship));
                break;
            case NET_SINK:
                printf("They shot at %c%i and sunk your %s!\n",
                    c + 'A',
                    r + 1,
                    ship_name(ship));
                break;
            case NET_MISS:
                printf("They shot at %c%i and missed.\n",
                    c + 'A',
                    r + 1);
                break;
            }

            if (outgoing.move_result.win) {
                printf("\nYou lost!\n");
                break;
            }

            printf("Press enter to continue...");
            skipline();
        }

        state.turn = state.turn == PEER_SERVER ? PEER_CLIENT : PEER_SERVER;
    }
}

static void server(const char* port) {
    int status;
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;

    if ((status = getaddrinfo("0.0.0.0", port, &hints, &res))) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        perror("socket error");
        exit(1);
    }

    // Reuse the port to prevent "already in use" errors
    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0) {
        perror("setsockopt error");
        exit(1);
    }

    if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("bind error");
        exit(1);
    }

    if (listen(sockfd, 1) < 0) {
        perror("listen error");
        exit(1);
    }

    // After binding, inspect the port and print it out
    {
        struct sockaddr_in new_addr;
        socklen_t new_addr_len = sizeof new_addr;
        if (getsockname(sockfd, (struct sockaddr*)&new_addr, &new_addr_len) < 0) {
            perror("getsockname error");
            exit(1);
        }

        if (new_addr.sin_family == AF_INET)
            printf("Server listening on port %i\n", ntohs(new_addr.sin_port));
    }

    struct sockaddr_storage caddr;
    socklen_t caddr_len = sizeof caddr;
    int cfd = accept(sockfd, (struct sockaddr*)&caddr, &caddr_len);
    if (cfd < 0) {
        perror("accept error");
        exit(1);
    }

    printf("Got client connection...\n");

    struct connection conn = { 
        .type = PEER_SERVER, 
        .is_disconnected = 0,
        .fd = cfd 
    };

    struct packet incoming, outgoing;
    
    EXPECT_PACKET(&conn, incoming, PKT_CLIENT_HELLO, "client hello");

    outgoing = (struct packet){ .type = PKT_SERVER_HELLO };
    send_packet(&conn, &outgoing);

    printf("Connected! When you're ready, press enter to begin.");
    skipline();

    outgoing = (struct packet){ .type = PKT_SERVER_READY };
    send_packet(&conn, &outgoing);

    play_game(&conn);
}

static void client(const char* host, const char* port) {
    int status;
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;

    if ((status = getaddrinfo(host, port, &hints, &res))) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        perror("socket error");
        exit(1);
    }

    if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("connect error");
        exit(1);
    }

    printf("Got server connection...\n");

    struct connection conn = {
        .type = PEER_CLIENT,
        .is_disconnected = 0,
        .fd = sockfd
    };

    struct packet incoming, outgoing;

    outgoing = (struct packet){ .type = PKT_CLIENT_HELLO };
    send_packet(&conn, &outgoing);
    
    EXPECT_PACKET(&conn, incoming, PKT_SERVER_HELLO, "server hello");

    printf("Connected! Waiting for host to begin...\n");

    EXPECT_PACKET(&conn, incoming, PKT_SERVER_READY, "server ready");

    play_game(&conn);
}

int main(int argc, const char** argv) {
    srand(time(NULL));

    if (argc >= 2 && strcmp(argv[1], "server") == 0) {
        server(argc > 2 ? argv[2] : NULL);
    } else if (argc >= 2 && strcmp(argv[1], "client") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s client <host> <port>\n", argv[0]);
            return 1;
        }

        client(argv[2], argv[3]);
    } else {
        fprintf(stderr, 
            "Run a server with: %s server [port]\n"
            "Connect to the server with: %s client <host> <port>\n", 
            argv[0], argv[0]);
        return 1;
    }
}