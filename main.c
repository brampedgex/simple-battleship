#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "board.h"
#include "player.h"

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

    printf("Client connected!\n");

    char msg[64] = {0};
    recv(cfd, msg, 63, 0);
    printf("client says: %s\n", msg);

    const char* reply = "Hello, client!";
    send(cfd, reply, strlen(reply), 0);
    printf("reply sent\n");
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

    const char* msg = "Hello, server\n";
    send(sockfd, msg, strlen(msg), 0);

    char reply[64] = {0};
    recv(sockfd, reply, 63, 0);
    printf("Server reply: %s\n", reply);
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