#include "ucp.h"
#include "rcs.h"

int rcsSocket() {
    return ucpSocket();
}

int rcsBind(int sockfd; struct sockaddr_in * addr) {
    return ucpBind(sockfd, addr);
}

int rcsGetSockName(int sockfd; struct sockaddr_in * addr) {
    return ucpGetSockName(sockfd, addr);
}

int rcsListen(int sockfd) {
    // udp no listen, what do

    // user get sock name to get the addr

    return 0;
}

int rcsAccept(int sockfd, struct sockaddr_in ∗ addr) {
    handshake hs1;
    // receive shits

    // choose init seq num, y

    // send TCP SYNACK msg, acking SYN


    // received ACK(y)

    // indicates client is live
}

int rcsConnect(int sockfd, struct sockaddr_in ∗ addr) {
    handshake hs1;
    // choose init seq num, x
    hs1.seq = 11;

    // send TCP SYN msg
    hs1.syn_bit = 1;
    ucpSendTo(sockfd, &hs1, sizeof(hs1), addr);


    // received SYNACK(x)

    // indicates server is live;

    // send ACK for SYNACK;

    // this segment may contain client-to-server data
}
