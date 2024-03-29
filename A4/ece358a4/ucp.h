#ifndef __UCP__
#define __UCP__

#include <unistd.h>

unsigned int get_rand();
int ucpSocket();
int ucpBind(int sockfd, struct sockaddr_in *addr);
int ucpGetSockName(int sockfd, struct sockaddr_in *addr);
int ucpSetSockRecvTimeout(int sockfd, int milliSecs);
int ucpSendTo(int sockfd, const void *buf, int len, const struct sockaddr_in *to);
ssize_t ucpRecvFrom(int sockfd, void *buf, int len, struct sockaddr_in *from);
int ucpClose(int sockfd);

#endif
