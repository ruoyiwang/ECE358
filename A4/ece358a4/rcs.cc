#include "ucp.c"
#include "rcs.h"
#include <map>

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
    packet p2;
    
    initPacket(&p2);
    int y = getSynNum(sockfd);
    while (true) {
        // receive shits
        ucpRecvFrom(sockfd, &p2, sizeof(packet), addr);

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

        // received ACK(y)
        initPacket(&p2);
        ucpRecvFrom(sockfd, &p2, sizeof(packet), addr);
        if (!(p2.flags & ACK_BIT_MASK) || p2.ack_num != y+1) {
            // if the ack bit is not set or wrong ack num start over
            continue;
        }
        else {
            // indicates client is live
            
            // make socket, bind it, and return it
            int new_socketfd = rcsSocket();
            rcsBind(new_socketfd, addr);
            return new_socketfd;
        }
    }

    // if it gets here somehow, then idk =___=
    return -1;
}

int rcsConnect(int sockfd, struct sockaddr_in * addr) {
    ucpSetSockRecvTimeout(sockfd, TIME_OUT);
    sockaddr_in recv_addr;
    packet p1;
    int x = getSynNum(sockfd);
    int tries = 5;
    while(tries--) {
        initPacket(&p1);
        // choose init seq num, x
        p1.seq_num = x;

        // send TCP SYN msg
        p1.flags = p1.flags | SYN_BIT_MASK;
        ucpSendTo(sockfd, &p1, sizeof(packet), addr);
        
        // received SYNACK(x)
        initPacket(&p1);
        if (ucpRecvFrom(sockfd, &p1, sizeof(packet), &recv_addr) == -1) {
            continue;
        }
        // indicates server is live;


        // send ACK for SYNACK
        ack_for_server = p1.seq_num + 1;
        initPacket(&p1);
        p1.flags = p1.flags | ACK_BIT_MASK;
        p1.ack_num = ack_for_server;
        if (ucpRecvFrom(sockfd, &p1, sizeof(packet), &recv_addr) == -1) {
            return 0;
        }
    }

    // "timed out"
    // TODO: set some err flag or some shit
    return -1;
}

