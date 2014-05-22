#include <stdio.h>
#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <ifaddrs.h>
#include <cstdlib>
#include <cstddef>

using namespace std;

int main (int argc, char *argv[]) {
    if (argc < 3) {
        // check the amount of the args on input, if too little, cerr and exit
        cerr << "Usage: " << argv[0] << "<servername/ip> <port>" <<endl;
        exit(0);
    }

    char* serverNameOrIp = argv[1];
    char* serverPort     = argv[2];

    int clientSocketFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocketFileDescriptor < 0) {
        cerr << "socket creation failed" << endl;
        exit(0);
    }

    // make the local addr
    struct sockaddr_in localSockAddr;
    localSockAddr.sin_family = AF_INET;
    localSockAddr.sin_addr.s_addr = INADDR_ANY; // don't care
    localSockAddr.sin_port = htons(0);

    // make the server sock addr in
    struct sockaddr_in serverSockAddr;
    memset(&serverSockAddr, 0, sizeof(serverSockAddr));

    //  lookup domain name and convert is to ip
    // this code's credit goes to the 358 TA
    struct addrinfo *res, hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo (serverNameOrIp, NULL, (&hints), &res) != 0) {
        cerr<< "so getaddrinfo failed"<<endl;
        exit(0);
    }

    struct addrinfo *cai;
    for (cai = res; cai != NULL; cai = cai->ai_next) {
        if (cai->ai_family == AF_INET) {
            memcpy (&serverSockAddr, cai->ai_addr, sizeof(struct sockaddr_in));
            cout << "Found AF_INET" << endl;
        }
    }
    serverSockAddr.sin_family = AF_INET;
    serverSockAddr.sin_port = htons(atoi(serverPort));

    char buf[128] = {0};
    if (!inet_ntop(serverSockAddr.sin_family, (void *)&(serverSockAddr.sin_addr), buf, sizeof(buf))) {
        // printf("%s: inet_ntop failed!\n", ifa->ifa_name);
    }
    else {
        printf("%s ", buf);
    }

    // try to connect to that shit
    if (connect(clientSocketFileDescriptor, (const struct sockaddr *)&serverSockAddr, sizeof(serverSockAddr)) <  0 ) {
        perror("connection fails");
        exit(0);
    }

    int whateverThisNumIs = sendto(clientSocketFileDescriptor, "test msg", strlen("test msg") + 1, 0, (const struct sockaddr *)(&serverSockAddr), sizeof(serverSockAddr));
    if (whateverThisNumIs < 0) {
        perror("err writing to socket");
    }
    cout<<"so apparently this worked, and I sent out 'test msg', maybe?"<<endl;

    char receiveBuffer[256] = {0};

    int responseSize = recvfrom(clientSocketFileDescriptor, receiveBuffer, 256, 0, NULL, NULL);
    if (responseSize < 0) {
        perror("failed receiving msg from server or some shit");
        exit(0);
    }

    cout<<receiveBuffer<<endl;


    // the parse / other shits starts here

    string queryStr;
    while (!getline(cin, queryStr).eof()) {
        const char * sendQuery = queryStr.c_str();

        int receiveSize = sendto(clientSocketFileDescriptor, sendQuery, strlen(sendQuery) + 1, 0, (const struct sockaddr *)(&serverSockAddr), sizeof(serverSockAddr));
        if (receiveSize < 0) {
            perror("err writing to socket");
        }
        cout<<"so apparently this worked, and I sent out 'test msg', maybe?"<<endl;

        char receiveBuff[256] = {0};
        int responseSize = recvfrom(clientSocketFileDescriptor, receiveBuff, 256, 0, NULL, NULL);
        if (responseSize < 0) {
            perror("failed receiving msg from server or some shit");
            exit(0);
        }
        cout<<receiveBuffer<<endl;
    }
    // TODO: send STOP_SESSION


    return 0;
}


// reference: http://pubs.opengroup.org/onlinepubs/000095399/basedefs/netinet/in.h.html

