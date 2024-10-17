#include "network.h"
#include "packet.h"
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <stdarg.h>
#include <stdio.h>

static char* pack_u16(char* ptr, u16 num) {
    ptr[0] = (char)(num >> 8);
    ptr[1] = (char)(num & 0xFF);
    return ptr + 2;
}

static char* pack_u32(char* ptr, u32 num) {
    ptr[0] = (char)(num >> 24);
    ptr[1] = (char)((num >> 16) & 0xFF);
    ptr[2] = (char)((num >> 8) & 0xFF);
    ptr[3] = (char)(num & 0xFF);
    return ptr + 4;
}

static char* pack_header(char* ptr, struct packet_header* header) {
    *ptr = (char)(header->type);
    return pack_u16(ptr + 1, (uint16_t)(header->length));
}

static char* unpack_u16(char* ptr, u16* num) {
    *num = (((u8)ptr[0]) << 8) | ((u8)ptr[1]);
    return ptr + 2;
}

static char* unpack_u32(char* ptr, u32* num) {
    *num = (((u8)ptr[0]) << 24) 
            | (((u8)ptr[1]) << 16)
            | (((u8)ptr[2]) << 8)
            | ((u8)ptr[3]);
    return ptr + 4;
}

static char* unpack_header(char* ptr, struct packet_header* header) {
    header->type = (enum packet_type)*ptr;
    return unpack_u16(ptr + 1, &header->length);
}

void disconnectf(struct connection* conn, const char* fmt, ...) {
    if (conn->is_disconnected)
        return;
    
    struct packet packet = { 
        .type = PKT_DISCONNECT,
        .disconnect = {
            .reason = {0}
        }
    };

    va_list args;
    va_start(args, fmt);
    vsnprintf(packet.disconnect.reason, 512, fmt, args);
    va_end(args);

    fprintf(stderr, "disconnecting peer: %s\n", packet.disconnect.reason);

    send_packet(conn, &packet);
    conn->is_disconnected = 1;
}

#define PACKET_MAX_LENGTH 512

void send_packet(struct connection *conn, struct packet *pkt) {
    struct packet_header header = { .type = pkt->type };

    char buf[3 + PACKET_MAX_LENGTH];
    char* body = &buf[3];
    
    switch (pkt->type) {
    case PKT_SERVER_HELLO:
    case PKT_CLIENT_HELLO:
        body = pack_u32(body, NET_MAGIC);
        break;
    case PKT_SERVER_READY:
    case PKT_SHIPS_READY:
        break;
    case PKT_DISCONNECT: {
        size_t length = strlen(pkt->disconnect.reason);
        memcpy(body, pkt->disconnect.reason, length);
        body += length;
    } break;
    case PKT_BEGIN_GAME:
        *body++ = (u8)pkt->begin_game.first;
        break;
    case PKT_MOVE:
        *body++ = (u8)pkt->move.row;
        *body++ = (u8)pkt->move.col;
        break;
    case PKT_MOVE_RESULT:
        *body++ = (u8)pkt->move_result.result;
        break;
    case PKT_END_GAME:
        *body++ = (u8)pkt->end_game.winner;
        break;
    default:
        fprintf(stderr, "TODO: Packet type %i\n", pkt->type);
        return;
    }

    header.length = body - buf - 3;
    pack_header(buf, &header);
    send(conn->fd, buf, header.length + 3, 0);
}

int recv_packet(struct connection* conn, struct packet* pkt) {
    ssize_t recv_status;

    char buf[3 + PACKET_MAX_LENGTH];
    if ((recv_status = recv(conn->fd, buf, 3, 0)) < 3) {
        if (recv_status == 0) {
            fprintf(stderr, "error: The connection was closed.\n");
            return -1;
        }
        if (recv_status < 0) {
            perror("recv error");
            return -1;
        }
        disconnectf(conn, "protocol error: bad packet header");
        return -1;
    }

    struct packet_header header;
    unpack_header(buf, &header);

    if (header.length > PACKET_MAX_LENGTH) {
        disconnectf(conn, "protocol error: packet too long");
        return -1;
    }

    char* body = buf + 3;
    if (header.length > 0) {
        if ((recv_status = recv(conn->fd, body, header.length, 0)) < header.length) {
            if (recv_status < 0) {
                perror("recv error");
                return -1;
            }
            disconnectf(conn, "protocol error: didn't get all the bytes (%i of %i)", (int)recv_status, header.length);
            return -1;
        }
    }

#define EXPECT_LENGTH(elength, name)                                                     \
    if (header.length != (elength)) {                                                    \
        disconnectf(conn, "protocol error: bad " name " length: %i", header.length);    \
        return -1;                                                                      \
    }


    switch (header.type) {
    case PKT_CLIENT_HELLO: {
        EXPECT_LENGTH(4, "client hello");

        u32 magic;
        unpack_u32(body, &magic);
        if (magic != NET_MAGIC) {
            disconnectf(conn, "protocol error: bad magic");
            return -1;
        }
    } break;
    case PKT_SERVER_HELLO: {
        EXPECT_LENGTH(4, "server hello");

        u32 magic;
        unpack_u32(body, &magic);
        if (magic != NET_MAGIC) {
            disconnectf(conn, "protocol error: bad magic");
            return -1;
        }
    } break;
    case PKT_SERVER_READY:
        EXPECT_LENGTH(0, "server ready");
        break;
    case PKT_SHIPS_READY:
        EXPECT_LENGTH(0, "ships ready");
        break;
    case PKT_DISCONNECT:
        if (header.length > 511) {
            // TODO: it's kinda stupid to disconnect when receiving a disconnect message
            disconnectf(conn, "protocol error: disconnect message too long");
            return -1;
        }

        // TODO: Maybe, just maybe, we should sanitize the string
        strncpy(pkt->disconnect.reason, body, 511);
        break;
    case PKT_BEGIN_GAME: {
        EXPECT_LENGTH(1, "begin game");
        enum peer_type first;
        first = (enum peer_type)*body;
        if (first != PEER_CLIENT && first != PEER_SERVER) {
            disconnectf(conn, "protocol error: invalid 'first' field in begin game");
            return -1;
        }

        pkt->begin_game.first = first;
    } break;
    case PKT_MOVE: {
        EXPECT_LENGTH(2, "move");

        int row = (int)body[0];
        int col = (int)body[1];
        if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
            disconnectf(conn, "protocol error: invalid coordinates in move packet");
            return -1;
        }

        pkt->move.row = row;
        pkt->move.col = col;
    } break;
    case PKT_MOVE_RESULT: {
        EXPECT_LENGTH(1, "move result");
        
        enum net_move_result result = (enum net_move_result)*body;
        switch (result) {
        case NET_HIT:
        case NET_MISS:
        case NET_SINK:
            break;
        default:
            disconnectf(conn, "protocol error: invalid result in move result packet");
            return -1;
        }

        pkt->move_result.result = result;
        break;
    } case PKT_END_GAME:
        EXPECT_LENGTH(1, "end game");
        break;
    default:
        disconnectf(conn, "protocol error: bad packet type: %i", header.type);
        return -1;
    }

    pkt->type = header.type;
    return 0;
}

/* int net_get_header(int fd, struct packet_header* header) {
    char buf[3];
    if (recv(fd, buf, 3, 0) < 3)
        return -1;

    unpack_header(buf, header);
    return 0;
}

int net_get_client_hello(struct connection* conn) {

} */