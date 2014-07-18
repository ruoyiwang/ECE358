#include <stdio.h>
#include <string.h>
#include "ucp.c"
#include "rcs.h"
#include <map>
#include <iostream>
#include <mybind.c>

using namespace std;

int ack_for_server;
// sockaddr_in server_addr; // for client to track the server
map<int, int> listening_sockets;
map<int, int> client_server_sockets;
map<int, sockaddr_in> server_addr_map;

void initPacket(packet* p) {
    p->ack_num = -1;
    p->seq_num = -1;
    p->checksum = 0;
    p->buff_len = 0;
    p->type = 0;
    p->flags = 0;

    return;
}

int lookup_addr_map(int sockfd, struct sockaddr_in * addr) {
    // check if we ever mapped it
    if (server_addr_map.find(sockfd) == server_addr_map.end()) {
        return -1;
    }

    // copy the data from map into addr so stuff can use it
    memcpy(addr, &(server_addr_map[sockfd]), sizeof(sockaddr_in));

    return 0;
}

int map_addr(int sockfd, struct sockaddr_in * addr) {
    sockaddr_in temp_addr;
    memcpy(&temp_addr, addr, sizeof(sockaddr_in));
    server_addr_map[sockfd] = temp_addr;

    return 0;
}

int getSynNum(int sockfd) {
    static int acknum = 0;
    return (acknum++) % SYN_NUM_MAX;
}

int getCheckSum(char * buf, int len) {
    int i;
    int ret = 0;
    for ( i = 0; i < len ; i++) {
        ret += (int)*(buf+i);
    }
    return ret;
}

int rcsSocket() {
    int sockfd = ucpSocket();
    if (sockfd < 0) {
        // ucp socket should have set flags
        return -1;
    }

    // remembers we made this socket
    listening_sockets[sockfd] = 0;
    return sockfd;
}

int rcsBind(int sockfd, struct sockaddr_in * addr) {
    return ucpBind(sockfd, addr);
}

int rcsGetSockName(int sockfd, struct sockaddr_in * addr) {
    return ucpGetSockName(sockfd, addr);
}

int rcsListen(int sockfd) {
    // check the flags and stuff
    // check ENOTSOCK
    if (listening_sockets.find(sockfd) == listening_sockets.end()) {
        errno = ENOTSOCK;
        return -1;
    }
    // check EBADF
    sockaddr_in temp;
    if (rcsGetSockName(sockfd, &temp) < 0) {
        errno = EBADF;
        return -1;
    }
    // check EADDRINUSE
    map<int, sockaddr_in>::iterator it = server_addr_map.begin();
    for (; it != server_addr_map.end(); it++) {
        if (it->second.sin_port == temp.sin_port) {
            errno = EADDRINUSE;
            return -1;
        }
    }

    // check EOPNOTSUPP, ignore this

    // if never listened before
    listening_sockets[sockfd] = 1;
    return 0;

}

int rcsAccept(int sockfd, struct sockaddr_in * addr) {
    // must be listening first
    if (listening_sockets[sockfd] != 1) {
        return -1;
    }

    ucpSetSockRecvTimeout(sockfd, 0);

    packet p2;

    initPacket(&p2);
    int y = getSynNum(sockfd);

    // the port used for comm
    int new_socketfd = rcsSocket();
    // binds to a local port
    sockaddr_in new_sockaddr;
    new_sockaddr.sin_family = AF_INET;
    new_sockaddr.sin_port = 0;
    new_sockaddr.sin_addr.s_addr = INADDR_ANY;
    if (rcsBind(new_socketfd, &new_sockaddr) < 0) {
        cerr << "error binding to new port" <<endl;
    }
    rcsListen(new_socketfd);

    while (true) {
        // receive shits
        int received_len = ucpRecvFrom(sockfd, &p2, sizeof(packet), addr);

        if (!(p2.flags & SYN_BIT_MASK)) {
            // if the syn bit is not set, start over
            continue;
        }

        // choose init seq num, y
        p2.flags = p2.flags | ACK_BIT_MASK;
        p2.ack_num = p2.seq_num + 1;
        p2.seq_num = y;
        // send TCP SYNACK msg, acking SYN with new port
        ucpSendTo(new_socketfd, &p2, sizeof(packet), addr);

        // received ACK(y)
        initPacket(&p2);
        // need to start listening on both ports
        ucpSetSockRecvTimeout(sockfd, TIME_OUT);
        ucpSetSockRecvTimeout(new_socketfd, TIME_OUT);
        while (
            ucpRecvFrom(sockfd, &p2, sizeof(packet), addr) < 0 &&
            ucpRecvFrom(new_socketfd, &p2, sizeof(packet), addr) < 0
        ) {}

        while (!(p2.flags & ACK_BIT_MASK) || p2.ack_num != y+1) {
            // if the ack bit is not set or wrong ack num start over
            if (p2.ack_num == -1) {
                // client restarting process
                initPacket(&p2);
                break;
            }
            initPacket(&p2);
            while (
                ucpRecvFrom(sockfd, &p2, sizeof(packet), addr) < 0 &&
                ucpRecvFrom(new_socketfd, &p2, sizeof(packet), addr) < 0
            ) {}
        }
        if (p2.ack_num == -1) {
            // broke out of the inner loop because wanna restart
            continue;
        }
        // indicates client is live

        // make socket, bind it, and return it
        map_addr(new_socketfd, addr);
        client_server_sockets[new_socketfd] = sockfd;
        return new_socketfd;
    }

    // if it gets here somehow, then idk =___=
    return -1;
}

int rcsConnect(int sockfd, const struct sockaddr_in * addr) {
    sockaddr_in addr_temp, server_addr;
    if (rcsGetSockName(sockfd, &addr_temp) < 0) {
        cerr << "sock has not been binded yet" << endl;
        return -1;
    }

    int packet_size = sizeof(packet);

    ucpSetSockRecvTimeout(sockfd, TIME_OUT);
    packet p1;
    int x = getSynNum(sockfd);
    int tries = 5;
    while(true) {
        initPacket(&p1);
        // choose init seq num, x
        p1.seq_num = x;

        // send TCP SYN msg
        p1.flags = p1.flags | SYN_BIT_MASK;
        while (ucpSendTo(sockfd, &p1, packet_size, addr) < packet_size) {}
        // if the packet I'm sending is too small, we just try again

        // received SYNACK(x)
        initPacket(&p1);
        if (ucpRecvFrom(sockfd, &p1, sizeof(packet), &server_addr) < 0) {
            continue;
        }
        if (!(p1.flags & ACK_BIT_MASK) || p1.ack_num != x+1) {
            continue;
        }
        if (!(p1.flags & SYN_BIT_MASK)) {
            continue;
        }
        map_addr(sockfd, &server_addr);
        // indicates server is live;


        // send ACK for SYNACK
        ack_for_server = p1.seq_num + 1;
        initPacket(&p1);
        p1.flags = p1.flags | ACK_BIT_MASK;
        p1.ack_num = ack_for_server;
        // while (ucpSendTo(sockfd, &p1, packet_size, addr) < packet_size) {}
        // if the packet is too small, we try again
        if (ucpSendTo(sockfd, &p1, packet_size, addr) >= packet_size) {
            return true;
        }
    }

    // "timed out"
    // TODO: set some err flag or some shit
    return -1;
}

int rcsRecv(int sockfd, void * buf, int len) {
    char * current_partition = (char *) buf;
    int expected_seq = 0;
    packet p1, p2;
    sockaddr_in client_addr; // for server to track the server
    int bad_packet = FALSE;
    int terminated = FALSE;
    int server_sockfd = client_server_sockets[sockfd];
    int ret = 0;
    // test line
    server_sockfd = sockfd;

    //set the time out of hte Recv to block indefinitely
    ucpSetSockRecvTimeout(server_sockfd, 0);
    for (;;) {
        initPacket(&p1);
        // if the termintating flag is set, then send the terminating ack to sender then return
        if (terminated) {
            for (int i = 0; i < 10; i ++) {
                p2.flags = p2.flags | END_BIT_MASK;
                p2.ack_num = TERM_ACK;
                ucpSendTo(server_sockfd, &p2, sizeof(packet), &client_addr);
            }
            return ret;
        }

        // receive from sender
        ucpRecvFrom(server_sockfd, &p1, sizeof(packet), &client_addr);

        // check close is called by the sender if so return -1
        if (p1.ack_num == CLOSE_ACK && p1.flags & END_BIT_MASK) {
            ucpSetSockRecvTimeout(server_sockfd, 1);
            for (int j = 0; j < 12; j ++) {
                ucpRecvFrom(server_sockfd, &p1, sizeof(packet), &client_addr);
            }
            return -1;
        }

        // the packet is bad if the sequence number is not expected, and the checksum is wrong
        if ((p1.seq_num != expected_seq)
            || (getCheckSum(p1.buff, p1.buff_len) != p1.checksum)) {
            bad_packet = 1;
        }

        // if a bad packet is recieved than send back a duplicate ack
        if (bad_packet){
            initPacket(&p2);
            p2.ack_num = p1.seq_num;
            if (p1.seq_num == expected_seq){
                p2.ack_num = expected_seq - 1;
            }
            p2.buff_len = 0;
            p1.flags = p1.flags | ACK_BIT_MASK;
            ucpSendTo(server_sockfd, &p2, sizeof(packet), &client_addr);
        }
        // if a good ack is received
        else {
            if (expected_seq == 0){
                ucpSetSockRecvTimeout(server_sockfd, TIME_OUT);
            }
            initPacket(&p2);
            p2.ack_num = expected_seq;
            p2.buff_len = 0;
            p1.flags = p1.flags | ACK_BIT_MASK;

            // calculate the return value aka number of bytes processed
            ret = expected_seq * BUFFER_SIZE + p1.buff_len;
            if (ret > len) {
                ret = len;
            }

            // if the send has finised sending or if the receive len run out
            if (p1.flags & END_BIT_MASK || ret == len) {
                // set the termintating flag
                p2.ack_num = TERM_ACK;
                p2.flags = p2.flags | END_BIT_MASK;
                ucpSendTo(server_sockfd, &p2, sizeof(packet), &client_addr);
                terminated = TRUE;
                current_partition = (char *)buf + expected_seq * BUFFER_SIZE;
                memcpy (current_partition, p1.buff, ret - expected_seq * BUFFER_SIZE);
                continue;
            }
            // add the recieved message to the buffer, then send back an corresponding ack
            current_partition = (char *)buf + expected_seq * BUFFER_SIZE;
            memcpy (current_partition, p1.buff, p1.buff_len);
            expected_seq = expected_seq + 1;
            ucpSendTo(server_sockfd, &p2, sizeof(packet), &client_addr);
        }
        bad_packet = 0;

    }
    return -1;
}

int rcsSend(int sockfd, const void* buf, int len) {
    char * current_partition = (char *) buf;
    int i, buff_len, expected_ack = 0;
    int seq_window_start = 0;
    packet p1, p2;
    // set the timout for receive of ack
    ucpSetSockRecvTimeout(sockfd, TIME_OUT);

    sockaddr_in server_addr;
    lookup_addr_map(sockfd, &server_addr);
    int k;

    for(;;) {
        k = 0;
        // send the window size amount of packets
        for (i = 0; i < WINDOW_SIZE; i ++) {
            initPacket(&p1);
            buff_len = BUFFER_SIZE;
            // get the current packet location in the buffer
            current_partition = (char *)buf + (seq_window_start + i) * BUFFER_SIZE;
            // if this is the last packet then add the end flag
            if ( (current_partition + BUFFER_SIZE - (char *)buf) >= len ) {
                buff_len = ((char*)buf + len - current_partition);
                p1.flags = p1.flags | END_BIT_MASK;
            }
            // set the proper sequence number
            p1.seq_num = (seq_window_start + i) % SYN_NUM_MAX;
            p1.ack_num = ack_for_server;
            // copy over the buffer to the packet
            memcpy(p1.buff, current_partition, buff_len);
            p1.checksum = getCheckSum(p1.buff, buff_len);
            p1.buff_len = buff_len;
            p1.flags = p1.flags | ACK_BIT_MASK;
            p1.flags = p1.flags | SYN_BIT_MASK;
            // send
            ucpSendTo(sockfd, &p1, sizeof(packet), &server_addr);
            k++;
            if (p1.flags & END_BIT_MASK) {
                break;
            }
        }
        // recieve the acks
        for (i = 0; i < WINDOW_SIZE; i ++) {
            initPacket(&p2);
            // receive
            ucpRecvFrom(sockfd, &p2, sizeof(packet), &server_addr);
            // if close than return -1
            if (p2.ack_num == CLOSE_ACK && p2.flags & END_BIT_MASK) {
                for (int j = 0; j < 12; j ++) {
                    ucpRecvFrom(sockfd, &p2, sizeof(packet), &server_addr);
                }
                return -1;
            }
            // if terminating ack is received then return
            if (p2.ack_num == TERM_ACK && p2.flags & END_BIT_MASK) {
                for (int j = 0; j < 12; j ++) {
                    ucpRecvFrom(sockfd, &p2, sizeof(packet), &server_addr);
                }
                return 0;
            }
            // if the ack is wrong the skip
            if (p2.ack_num != expected_ack) {
                continue;
            }
            // if the ack is right increase the expected ack as well as the window
            expected_ack = (expected_ack + 1) % SYN_NUM_MAX;
            seq_window_start = (seq_window_start + 1) % SYN_NUM_MAX;
        }
    }
    return -1;
}

int rcsClose(int sockfd) {
    sockaddr_in addr;
    packet p1;
    initPacket(&p1);
    lookup_addr_map(sockfd, &addr);
    // send 10 closing acks
    for (int i = 0; i < 10; i ++) {
        p1.flags = p1.flags | END_BIT_MASK;
        p1.ack_num = CLOSE_ACK;
        ucpSendTo(sockfd, &p1, sizeof(packet), &addr);
    }
    return 0;
}