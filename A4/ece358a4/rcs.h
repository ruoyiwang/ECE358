#ifndef __RCS__
#define __RCS__

#define BUFFER_SIZE     128     // Bytes?
#define TIME_OUT        100    // ms, yeah idk how long is good lol

typedef struct packet_t {
    int ack_num;
    int checksum;
    void * buffer;
} packet;

typedef struct handshake_t {
    int syn_bit;
    int seq;
    int ack_bit;
    int ack_num;
} handshake;

int rcsSocket();
int rcsBind(int sockfd; struct sockaddr_in * addr);
int rcsGetSockName(int sockfd; struct sockaddr in * addr);
int rcsListen(int sockfd);

int rcsAccept(int sockfd, struct sockaddr_in ∗ addr);
int rcsConnect(int sockfd, struct sockaddr_in ∗ addr);


#endif
