#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "board.h"
#include "packet.h"
#include "player.h"
#include "network.h"

static void server(const char* port) {
    int status;
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;

    if ((status = getaddrinfo("localhost", port, &hints, &res))) {
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

    printf("Waiting for client to connect...\n");

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
    if (recv_packet(&conn, &incoming)) {
        exit(1);
    }

    switch (incoming.type) {
    case PKT_CLIENT_HELLO:
        break;
    case PKT_DISCONNECT: {
        fprintf(stderr, "disconnected: %s\n", incoming.disconnect.reason);
        exit(0);
    } break;
    default:
        disconnectf(&conn, "unexpected initial packet from client");
        exit(1);
    }

    outgoing = (struct packet){ .type = PKT_SERVER_HELLO };
    send_packet(&conn, &outgoing);

    printf("Connected! When you're ready, press enter to begin.");
    getchar();

    outgoing = (struct packet){ .type = PKT_SERVER_READY };
    send_packet(&conn, &outgoing);

    // ...
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

    printf("Connected!\n");

    struct connection conn = {
        .type = PEER_CLIENT,
        .is_disconnected = 0,
        .fd = sockfd
    };

    struct packet incoming, outgoing;

    outgoing = (struct packet){ .type = PKT_CLIENT_HELLO };
    send_packet(&conn, &outgoing);
    
    if (recv_packet(&conn, &incoming)) {
        exit(1);
    }

    switch (incoming.type) {
    case PKT_DISCONNECT:
        fprintf(stderr, "disconnected: %s\n", incoming.disconnect.reason);
        break;
    case PKT_SERVER_HELLO:
        break;
    default:
        disconnectf(&conn, "expected a server hello packet");
        exit(1);
    }

    printf("Connected! Waiting for host to begin...\n");

    if (recv_packet(&conn, &incoming)) {
        exit(1);
    }

    switch (incoming.type) {
    case PKT_SERVER_READY:
        break;
    case PKT_DISCONNECT:
        fprintf(stderr, "disconnected: %s\n", incoming.disconnect.reason);
        break;
    default:
        disconnectf(&conn, "expected a server ready packet");
        exit(1);
    }

    // ...
}

int main(int argc, const char** argv) {
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