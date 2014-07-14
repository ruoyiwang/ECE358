#ifndef __RCS__
#define __RCS__

#include <netinet/in.h>

#define BUFFER_SIZE     128     // Bytes?
#define TIME_OUT        100    // ms, yeah idk how long is good lol
#define SYN_NUM_MAX     10000
#define WINDOW_SIZE     5
#define SYN_BIT_MASK    1
#define ACK_BIT_MASK    2

typedef struct packet_t {
    int ack_num;
    int seq_num;
    int checksum;
    char buff[BUFFER_SIZE];
    int buff_len;
    int type;
    char flags;
} packet;

// typedef struct handshake_t {
//     int syn_bit;
//     int seq;
//     int ack_bit;
//     int ack_num;
// } handshake;

int rcsSocket();
int rcsBind(int sockfd, struct sockaddr_in * addr);
int rcsGetSockName(int sockfd, struct sockaddr_in * addr);
int rcsListen(int sockfd);

int rcsAccept(int sockfd, struct sockaddr_in * addr);
int rcsConnect(int sockfd, struct sockaddr_in * addr);

#endif
