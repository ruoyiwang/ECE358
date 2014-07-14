#include "ucp.c"
#include "rcs.h"
#include <map>
#include <iostream>

using namespace std;

int ack_for_server;
map<int, int> listening_sockets;

void initPacket(packet* p) {
    p->ack_num = -1;
    p->seq_num = -1;
    p->checksum = 0;
    p->buff_len = 0;
    p->type = 0;
    p->flags = 0;

    return;
}

int getSynNum(int sockfd) {
    static int acknum = 0;
    return (acknum++) % SYN_NUM_MAX;
}

int rcsSocket() {
    return ucpSocket();
}

int rcsBind(int sockfd, struct sockaddr_in * addr) {
    return ucpBind(sockfd, addr);
}

int rcsGetSockName(int sockfd, struct sockaddr_in * addr) {
    return ucpGetSockName(sockfd, addr);
}

int rcsListen(int sockfd) {
    // check the flags and stuff
    if (listening_sockets.find(sockfd) == listening_sockets.end()) {
        // if never listened before
        listening_sockets[sockfd] = 1;
        return 0;
    }
    else if (listening_sockets.find(sockfd)->second != 1) {
        // if we ever somehow "unlistened", we can listen again
        listening_sockets[sockfd] = 1;
        return 0;
    }

    // else the port is listening... actually
    // TODO: set up status flag
    return -1;
}

int rcsAccept(int sockfd, struct sockaddr_in * addr) {
    // must be listening first
    if (listening_sockets[sockfd] != 1) {
        return -1;
    }

    packet p2;

    initPacket(&p2);
    int y = getSynNum(sockfd);
    while (true) {
        // receive shits
        cout << "waiting to receive first syn" << endl;
        ucpRecvFrom(sockfd, &p2, sizeof(packet), addr);
        cout << "received first syn" << endl;

        if (!(p2.flags & SYN_BIT_MASK)) {
            // if the syn bit is not set, start over
            continue;
        }

        // choose init seq num, y
        p2.flags = p2.flags | ACK_BIT_MASK;
        p2.ack_num = p2.seq_num + 1;
        p2.seq_num = y;
        // send TCP SYNACK msg, acking SYN
        ucpSendTo(sockfd, &p2, sizeof(packet), addr);
        cout << "sending first ack" << endl;

        // received ACK(y)
        initPacket(&p2);
        ucpRecvFrom(sockfd, &p2, sizeof(packet), addr);
        cout << "received second pkt" << endl;
        while (!(p2.flags & ACK_BIT_MASK) || p2.ack_num != y+1) {
            // if the ack bit is not set or wrong ack num start over
            cout << "wrong ack" << y+1 << "|" << p2.ack_num << endl;
            if (p2.ack_num == -1) {
                // client restarting process
                initPacket(&p2);
                break;
            }
            initPacket(&p2);
            ucpRecvFrom(sockfd, &p2, sizeof(packet), addr);
        }
        if (p2.ack_num == -1) {
            // broke out of the inner loop because wanna restart
            continue;
        }
        // indicates client is live

        // make socket, bind it, and return it
        int new_socketfd = rcsSocket();
        cout << "binding, portnum: " << addr->sin_port << endl;
        bind(new_socketfd, (const sockaddr *)addr, sizeof(struct sockaddr_in));
        return new_socketfd;
    }

    // if it gets here somehow, then idk =___=
    return -1;
}

int rcsConnect(int sockfd, struct sockaddr_in * addr) {
    sockaddr_in addr_temp;
    if (rcsGetSockName(sockfd, &addr_temp) < 0) {
        cerr << "sock has not been binded yet" << endl;
        return -1;
    }

    int packet_size = sizeof(packet);

    ucpSetSockRecvTimeout(sockfd, TIME_OUT);
    sockaddr_in recv_addr;
    packet p1;
    int x = getSynNum(sockfd);
    int tries = 5;
    while(true) {
        initPacket(&p1);
        // choose init seq num, x
        p1.seq_num = x;

        // send TCP SYN msg
        p1.flags = p1.flags | SYN_BIT_MASK;
        while (ucpSendTo(sockfd, &p1, packet_size, addr) < packet_size) {}
        // if the packet I'm sending is too small, we just try again
        cout << "sent first packet" <<endl;

        // received SYNACK(x)
        initPacket(&p1);
        if (ucpRecvFrom(sockfd, &p1, sizeof(packet), &recv_addr) < 0) {
            continue;
        }
        if (!(p1.flags & ACK_BIT_MASK) || p1.ack_num != x+1) {
            continue;
        }
        if (!(p1.flags & SYN_BIT_MASK)) {
            continue;
        }
        cout << "received first correct packet" <<endl;
        // indicates server is live;


        // send ACK for SYNACK
        ack_for_server = p1.seq_num + 1;
        initPacket(&p1);
        p1.flags = p1.flags | ACK_BIT_MASK;
        p1.ack_num = ack_for_server;
        // while (ucpSendTo(sockfd, &p1, packet_size, addr) < packet_size) {}
        // if the packet is too small, we try again
        cout << "sending second packet" <<endl;
        if (ucpSendTo(sockfd, &p1, packet_size, addr) >= packet_size) {
            return true;
        }
    }

    // "timed out"
    // TODO: set some err flag or some shit
    return -1;
}

int rcsRecv(int sockfd, void * buf, int len) {

}

int rcsSend(int sockfd, void* buf, int len) {

}