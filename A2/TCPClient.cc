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
    serverSockAddr.sin_family = AF_INET;
    serverSockAddr.sin_port = htons(atoi(serverPort));


    // TODO: lookup domain name and convert is to ip
    // struct hostent* host;
    // host = gethostbyname(serverNameOrIp);
    // cout<<host->h_addr<<endl;

    // convert ip from txt to bin
    if (inet_pton(AF_INET, serverNameOrIp, &serverSockAddr) <= 0) {
        cerr << "inet_pton fails" <<endl;
        exit(0);
    }

    // try to connect to that shit
    if (connect(clientSocketFileDescriptor, (struct sockaddr *)&serverSockAddr, sizeof(serverSockAddr)) <  0 ) {
        cerr << "connect fails" <<endl;
        exit(0);
    }

    int whateverThisNumIs = send(clientSocketFileDescriptor, "test msg", sizeof("test msg"), 0);
    if (whateverThisNumIs < 0) {
        cerr << "err writing to socket";
    }
    cout<<"so apparently this worked, and I sent out 'test msg', maybe?"<<endl;

    char receiveBuffer[256] = {0};

    int responseSize = recvfrom(clientSocketFileDescriptor, receiveBuffer, 256, 0, NULL, NULL);
    if (responseSize == -1) {
        cerr<<"failed receiving msg from server or some shit"<<endl;
        exit(0);
    }

    return 0;
}


// reference: http://pubs.opengroup.org/onlinepubs/000095399/basedefs/netinet/in.h.html