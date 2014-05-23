#include <stdio.h>
#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <ifaddrs.h>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <sstream>
#include <vector>
#include <locale>

using namespace std;

// tokenizes a string to a vector of string with space as delim
vector<string> tokenize(string input) {
    stringstream ss;
    ss << input;
    string temp;

    vector<string> vs;
    while (getline(ss, temp, ' ')) {
        if (temp != "") {
            vs.push_back(temp);
        }
    }

    return vs;
}

// check if a str is a number
bool IsNumber(string s) {
    locale l;
    for (int i = 0; i < s.length(); i++) {
        if (!isdigit(s[i], l)) {
            return false;
        }
    }
    return true;
}

// check if inputted str by user is valid
bool IsQueryStringValid(string queryStr) {
     vector<string> tokenizedQuery = tokenize(queryStr);
    if (tokenizedQuery.size() == 1 && tokenizedQuery.at(0) == "STOP") {
        return true;
    }
    else if (tokenizedQuery.size() == 2) {
        if (IsNumber(tokenizedQuery.at(0)) && IsNumber(tokenizedQuery.at(1))) {
            return true;
        }
        else {
            return false;
        }
    }
    return false;
}

int main (int argc, char *argv[]) {
    if (argc < 3) {
        // check the amount of the args on input, if too little, cerr and exit
        cerr << "Usage: " << argv[0] << "<servername/ip> <port>" <<endl;
        exit(0);
    }

    char* serverNameOrIp = argv[1];
    char* serverPort     = argv[2];

    int clientSocketFileDescriptor = socket(AF_INET, SOCK_DGRAM, 0);

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

    // lookup domain name and convert is to ip
    // this code's idea's credit goes to the 358 TA
    struct addrinfo *res, hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

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

    // don't need to run connect for udp

    string queryStr;
    while (!getline(cin, queryStr).eof()) {
        // have to parse queryStr
        if (!IsQueryStringValid(queryStr)) {
            cerr<<"error: invalid input"<<endl;
            continue;
        }
        vector<string> tokenizedQuery = tokenize(queryStr);

        // parsing worked 
        // if it's not STOP, get append GET in front of the command send to svr
        if (queryStr != "STOP") {
            // remade the queryStr so that it only has 1 space
            queryStr = tokenizedQuery.at(0) + " " + tokenizedQuery.at(1);
            queryStr = "GET " + queryStr;
        }
        const char * sendQuery = queryStr.c_str();

        // send the thingy to the server
        int receiveSize = sendto(clientSocketFileDescriptor, sendQuery, strlen(sendQuery) + 1, 0, (const struct sockaddr *)(&serverSockAddr), sizeof(serverSockAddr));
        if (receiveSize < 0) {
            perror("err writing to socket");
        }

        // after I send the stop I'll just close the file descriptor
        if (queryStr == "STOP") {
            close(clientSocketFileDescriptor);
            return 0;
        }

        // make the receive buffer and wait for the server to respond
        char receiveBuff[256] = {0};
        int responseSize = recvfrom(clientSocketFileDescriptor, receiveBuff, 256, 0, NULL, NULL);
        if (responseSize < 0) {
            perror("failed receiving msg from server");
            exit(0);
        }

        string receivedStr = string(receiveBuff);
        // make the possible error string returned by the sever
        string possibleError = "ERROR_" + queryStr;
        if (receivedStr == possibleError) {
            cerr << "error: " << queryStr << endl;
        }
        else {
            cout << receivedStr << endl; //" | Response Size: " << responseSize << endl;
        }
    }

    // the code breaks out of the loop is EOF is received
    // send STOP_SESSION
    int receiveSize = sendto(clientSocketFileDescriptor, "STOP_SESSION", strlen("STOP_SESSION") + 1, 0, (const struct sockaddr *)(&serverSockAddr), sizeof(serverSockAddr));
    if (receiveSize < 0) {
        perror("err writing to socket");
    }

    // don't care about the receive and return
    return 0;
}


// reference: http://pubs.opengroup.org/onlinepubs/000095399/basedefs/netinet/in.h.html
