#ifndef __RCS__
#define __RCS__

#include <netinet/in.h>
#define TRUE 		    1
#define FALSE       	0
#define TERM_ACK       	-2
#define CLOSE_ACK      	-3

#define BUFFER_SIZE     1000     // Bytes?
#define TIME_OUT        10   // ms, yeah idk how long is good lol
#define SYN_NUM_MAX     10000
#define WINDOW_SIZE     3
#define SYN_BIT_MASK    1
#define ACK_BIT_MASK    2
#define END_BIT_MASK    4

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
int getCheckSum(char * buf, int len);
int rcsSocket();
int rcsBind(int sockfd, struct sockaddr_in * addr);
int rcsGetSockName(int sockfd, struct sockaddr_in * addr);
int rcsListen(int sockfd);

int rcsAccept(int sockfd, struct sockaddr_in * addr);
int rcsConnect(int sockfd, const struct sockaddr_in * addr);
int rcsSend(int sockfd, const void* buf, int len);
int rcsRecv(int sockfd, void * buf, int len);
int rcsClose(int sockfd);

#endif
