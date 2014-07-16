#include "rcs.h"

// real stuff
#include <netinet/in.h>


// testing stuff
#include <cstring>
#include <iostream>
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
#include <iostream>
#include <sstream>
#include <map>
#include <vector>


using namespace std;

int main(int argc, char** argv) {
    if (argc < 1) {
        return 0;
    }
    if (string(argv[1]) == "server") {
        // run as server
        cout << argv[1] << endl;
        int sockfd = rcsSocket();

        rcsListen(sockfd);
        //////////////////////////////////////////////////////////////
        // initialize everything
        struct ifaddrs *myaddrs, *ifa;
        void *in_addr;
        char buf[256];
        int new_fd, portnum = 0;

        struct sockaddr_in my_addr, their_addr;
        socklen_t addrlen = sizeof(struct sockaddr_in);

        // get the ip information
        if(getifaddrs(&myaddrs) != 0)
        {
            perror("getifaddrs");
            exit(1);
        }

        // print the current host ip
        for (ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr == NULL)
                continue;
            if (!(ifa->ifa_flags & IFF_UP))
                continue;
            //only take it if tis ipv4 and not local
            if (ifa->ifa_addr->sa_family != AF_INET || strcmp(ifa->ifa_name, "lo") == 0) {
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

        // initialize the address struct
        my_addr.sin_family = AF_INET;
        my_addr.sin_port = 0;
        my_addr.sin_addr.s_addr = INADDR_ANY;

        // bind to a open port
        if (rcsBind (sockfd, (&my_addr)) < 0) {
            cerr << "failed biding server to port" << endl;
        }
        memset (&my_addr, 0, sizeof(struct sockaddr_in));

        // get the portnumber here

        if (getsockname(sockfd, (struct sockaddr *) (&my_addr), &addrlen) < 0) {
            perror ("getsockname");
            exit (0);
        }

        // print the port
        printf ("%hu\n", ntohs(my_addr.sin_port));


        ////////////////////////////////////////////

        cout << sockfd << endl;
        sockaddr_in sock_addr;
        sock_addr.sin_port = 0;
        rcsAccept(sockfd, &sock_addr);

        cout << "connection established" << endl;

        char buff[200000];
        for (int i = 0; i < 2000000/BUFFER_SIZE; i++){
            sprintf(buff + i*BUFFER_SIZE, "aaa");
            // strcpy(buff + i*BUFFER_SIZE, );
        }
        rcsRecv(sockfd, (void*) buff, 2000000);
        rcsRecv(sockfd, (void*) buff, 2000000);
        for (int i=0; i < 2000000 / BUFFER_SIZE; i ++){
            if ( atoi(buff+i*BUFFER_SIZE) != i ) {
                cout<<"section "<<i<<": "<<buff+i*BUFFER_SIZE<<endl;
                return 0;
            }
        }
    }
    else if (argc >= 3) {
        // run as client
        //////////////////////////////////////////////////////////////////////

        char* serverNameOrIp = argv[1];
        char* serverPort     = argv[2];

        // get file descriptor
        int clientSocketFileDescriptor = rcsSocket();

        if (clientSocketFileDescriptor < 0) {
            cerr << "socket creation failed" << endl;
            exit(0);
        }

        // make the local addr
        struct sockaddr_in localSockAddr;
        localSockAddr.sin_family = AF_INET;
        localSockAddr.sin_port = htons(0);
        localSockAddr.sin_addr.s_addr = INADDR_ANY;

        if (rcsBind (clientSocketFileDescriptor, (&localSockAddr)) < 0) {
            cerr << "failed biding client" << endl;
            return 0;
        }


        // make the server sock addr in
        struct sockaddr_in serverSockAddr;
        memset(&serverSockAddr, 0, sizeof(serverSockAddr));

        // lookup domain name and convert is to ip
        // this code's idea's credit goes to the 358 TA
        struct addrinfo *res, hints;
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        // get the addr info from eitehr the name or ip based on user input
        if (getaddrinfo(serverNameOrIp, NULL, (&hints), &res) != 0) {
            cerr<< "so getaddrinfo failed"<<endl;
            exit(0);
        }

        struct addrinfo *tempAddrInfo;

        // look for the addr info we want from the result of the addr info
        // copy it to our addr
        for (tempAddrInfo = res; tempAddrInfo != NULL; tempAddrInfo = tempAddrInfo->ai_next) {
            if (tempAddrInfo->ai_family == AF_INET) {
                memcpy (&serverSockAddr, tempAddrInfo->ai_addr, sizeof(struct sockaddr_in));
            }
        }

        // chage the family to AUY_INET
        serverSockAddr.sin_family = AF_INET;
        // write the server port in
        serverSockAddr.sin_port = htons(atoi(serverPort));

        // try to connect to the server
        if (rcsConnect(clientSocketFileDescriptor, &serverSockAddr) <  0 ) {
            perror("connection fails");
            exit(0);
        }

        cout << "connection established" << endl;
        ////////////////////////////////////////////
        char buff[2000000];
        for (int i = 0; i < 2000000/BUFFER_SIZE; i++){
            sprintf(buff + i*BUFFER_SIZE, "%d", i);
            // strcpy(buff + i*BUFFER_SIZE, );
        }
        rcsSend(clientSocketFileDescriptor, (void*) buff, 2000000);
        rcsSend(clientSocketFileDescriptor, (void*) buff, 2000000);
    }
    return 0;
}
