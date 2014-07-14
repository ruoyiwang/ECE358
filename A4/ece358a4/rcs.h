#ifndef __RCS__
#define __RCS__

int rcsSocket();
int rcsBind(int socket; struct sockaddr_in * sockaddr);
int rcsGetSockName(int sockfd; struct sockaddr in * addr);

#endif
