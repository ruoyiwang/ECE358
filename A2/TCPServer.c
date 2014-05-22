#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#include <netdb.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <string.h>

int main(int argc, char *argv[])
{
    struct ifaddrs *myaddrs, *ifa;
    void *in_addr;
    char buf[256];
    int new_fd, s, portnum = 0;

    struct sockaddr_in a, their_addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    
    if(getifaddrs(&myaddrs) != 0)
    {
        perror("getifaddrs");
        exit(1);
    }
    // print the local ip
    for (ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;
        if (!(ifa->ifa_flags & IFF_UP))
            continue;
        if (ifa->ifa_addr->sa_family != AF_INET || strcmp(ifa->ifa_name, "eth0") != 0) {
            continue;
        }

        struct sockaddr_in *s4 = (struct sockaddr_in *)ifa->ifa_addr;
        in_addr = &s4->sin_addr;

        if (!inet_ntop(ifa->ifa_addr->sa_family, in_addr, buf, sizeof(buf))) {
            printf("%s: inet_ntop failed!\n", ifa->ifa_name);
        }
        else {
            printf("%s ", buf);
        }
    }

    freeifaddrs(myaddrs);

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(0);
    }

    a.sin_family = AF_INET;
    a.sin_port = htons(portnum);
    a.sin_addr.s_addr = INADDR_ANY;

    while (bind (s, (const struct sockaddr *)(&a), sizeof(struct sockaddr_in)) < 0) {
        a.sin_port = htons(++portnum);
    }
    memset (&a, 0, sizeof(struct sockaddr_in));

    // In case you didnt specify the port number, we will get it here

    if (getsockname(s, (struct sockaddr *) (&a), &addrlen) < 0) {
        perror ("getsockname");
        exit (0);
    }

    printf ("Connecting to port %hu\n", ntohs(a.sin_port));
    //printf("%d\n", portnum);

    listen(s, 10); 


    memset(buf, 0, 256);
    memset(&a, 0, sizeof(struct sockaddr_in));

    new_fd = accept(s, (struct sockaddr *)&their_addr, &addrlen);

    if (recvfrom(new_fd, buf, 256, 0, (struct sockaddr*) (&a), &addrlen) < 0){
        perror("recvfrom");
    }

    printf("Server, received: %s\n", buf);
    return 0;
}
