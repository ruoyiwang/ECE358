#include "ucp.h"
#include "rcs.h"

int rcsSocket() {
    return ucpSocket();
}

int rcsBind(int sockfd; struct sockaddr_in * addr) {
    return ucpBind(sockfd, addr);
}

int rcsGetSockName(int sockfd; struct sockaddr in * addr) {
    return ucpGetSockName(sockfd, addr);
}
