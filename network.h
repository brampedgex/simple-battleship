#ifndef _NETWORK_H
#define _NETWORK_H

#include "packet.h"

struct connection {
    // Are we the server or the client?
    enum peer_type type;
    // 1 if we've disconnected
    int is_disconnected;
    int fd;
};

void disconnectf(struct connection* conn, const char* fmt, ...);

void send_packet(struct connection* conn, struct packet* pkt);
int recv_packet(struct connection* conn, struct packet* pkt);

#endif