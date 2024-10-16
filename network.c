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

void send_packet(struct connection *conn, struct packet *pkt) {
    struct packet_header header = { .type = pkt->type };
    
    switch (pkt->type) {
    case PKT_CLIENT_HELLO: {
        char buf[7];
        
        header.length = 4;
        pack_u32(pack_header(buf, &header), NET_MAGIC);
        send(conn->fd, buf, 7, 0);
    } break;
    case PKT_DISCONNECT: {
        char buf[3 + 511];

        header.length = strlen(pkt->disconnect.reason);
        char* ptr = pack_header(buf, &header);
        memcpy(ptr, pkt->disconnect.reason, header.length);

        send(conn->fd, buf, 3 + header.length, 0);
    } break;
    default:
        fprintf(stderr, "TODO: Packet type %i\n", pkt->type);
        return;
    }
}

int recv_packet(struct connection* conn, struct packet* pkt) {
    char buf[3];
    if (recv(conn->fd, buf, 3, 0) < 3) {
        disconnectf(conn, "protocol error: bad packet header");
        return -1;
    }

    struct packet_header header;
    unpack_header(buf, &header);

    switch (header.type) {
    case PKT_CLIENT_HELLO: {
        if (header.length != 4) {
            disconnectf(conn, "protocol error: bad client hello packet (1): %i", header.length);
            return -1;
        }

        char buf[4];
        if (recv(conn->fd, buf, 4, 0) < 4) {
            disconnectf(conn, "protocol error: bad client hello packet (2)");
            return -1;
        }

        u32 magic;
        unpack_u32(buf, &magic);
        if (magic != NET_MAGIC) {
            disconnectf(conn, "protocol error: bad magic");
            return -1;
        }
    } break;
    case PKT_DISCONNECT: {
        if (header.length > 511) {
            // TODO: it's kinda stupid to disconnect when receiving a disconnect message
            disconnectf(conn, "protocol error: disconnect message too long");
            return -1;
        }

        // TODO: Maybe, just maybe, we should sanitize the string
        char buf[512] = {0};
        if (recv(conn->fd, buf, header.length, 0) < header.length) {
            disconnectf(conn, "protocol error: bad disconnect packet");
            return -1;
        }
        strncpy(pkt->disconnect.reason, buf, 511);
    } break;
    default:
        disconnectf(conn, "protocol error: bad packet header");
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