#include <stdio.h>
#include <string.h>
#include "ucp.c"
#include "rcs.h"
#include <map>
#include <iostream>

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

    packet p2;

    initPacket(&p2);
    int y = getSynNum(sockfd);
    while (true) {
        // receive shits
        cout << "waiting to receive first syn" << endl;
        ucpRecvFrom(sockfd, &p2, sizeof(packet), addr);
        cout << "received first syn" << endl;

        if (!(p2.flags & SYN_BIT_MASK)) {
            // if the syn bit is not set, start over
            continue;
        }

        // choose init seq num, y
        p2.flags = p2.flags | ACK_BIT_MASK;
        p2.ack_num = p2.seq_num + 1;
        p2.seq_num = y;
        // send TCP SYNACK msg, acking SYN
        ucpSendTo(sockfd, &p2, sizeof(packet), addr);
        cout << "sending first ack" << endl;

        // received ACK(y)
        initPacket(&p2);
        ucpRecvFrom(sockfd, &p2, sizeof(packet), addr);
        cout << "received second pkt" << endl;
        while (!(p2.flags & ACK_BIT_MASK) || p2.ack_num != y+1) {
            // if the ack bit is not set or wrong ack num start over
            cout << "wrong ack" << y+1 << "|" << p2.ack_num << endl;
            if (p2.ack_num == -1) {
                // client restarting process
                initPacket(&p2);
                break;
            }
            initPacket(&p2);
            ucpRecvFrom(sockfd, &p2, sizeof(packet), addr);
        }
        if (p2.ack_num == -1) {
            // broke out of the inner loop because wanna restart
            continue;
        }
        // indicates client is live

        // make socket, bind it, and return it
        int new_socketfd = rcsSocket();
        cout << "binding, portnum: " << addr->sin_port << endl;
        // bind(new_socketfd, (const sockaddr *)addr, sizeof(struct sockaddr_in));
        map_addr(new_socketfd, addr);
        client_server_sockets[new_socketfd] = sockfd;
        return new_socketfd;
    }

    // if it gets here somehow, then idk =___=
    return -1;
}

int rcsConnect(int sockfd, struct sockaddr_in * addr) {
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
        cout << "sent first packet" <<endl;

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
        cout << "received first correct packet" <<endl;
        // indicates server is live;


        // send ACK for SYNACK
        ack_for_server = p1.seq_num + 1;
        initPacket(&p1);
        p1.flags = p1.flags | ACK_BIT_MASK;
        p1.ack_num = ack_for_server;
        // while (ucpSendTo(sockfd, &p1, packet_size, addr) < packet_size) {}
        // if the packet is too small, we try again
        cout << "sending second packet" <<endl;
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

    ucpSetSockRecvTimeout(server_sockfd, TIME_OUT);
    for (;;) {
        initPacket(&p1);
        ucpRecvFrom(server_sockfd, &p1, sizeof(packet), &client_addr);

        if (p1.ack_num == CLOSE_ACK && p1.flags & END_BIT_MASK) {
            for (int j = 0; j < 12; j ++) {
                ucpRecvFrom(server_sockfd, &p1, sizeof(packet), &client_addr);
            }
            return -1;
        }

        if (terminated) {
            // cout<< "terminating "<<expected_seq<<endl;
            for (int i = 0; i < 10; i ++) {
                p2.flags = p2.flags | END_BIT_MASK;
                p2.ack_num = TERM_ACK;
                ucpSendTo(server_sockfd, &p2, sizeof(packet), &client_addr);
            }
            return 0;
        }

        if ((p1.seq_num != expected_seq)
            || (getCheckSum(p1.buff, p1.buff_len) != p1.checksum)) {
            bad_packet = 1;
        }
        // cout<< "received seq#"<< p1.seq_num<<" expecting "<<expected_seq<<endl;

        if (bad_packet){
            // cout<< "bad_packet"<< expected_seq<<endl;
            initPacket(&p2);
            p2.ack_num = p1.seq_num;
            if (p1.seq_num == expected_seq){
                p2.ack_num = expected_seq - 1;
            }
            p2.buff_len = 0;
            p1.flags = p1.flags | ACK_BIT_MASK;
            ucpSendTo(server_sockfd, &p2, sizeof(packet), &client_addr);
        }
        else {
            // cout<< "good_packet"<< expected_seq<<endl;
            initPacket(&p2);
            p2.ack_num = expected_seq;
            p2.buff_len = 0;
            p1.flags = p1.flags | ACK_BIT_MASK;
            //proccess

            // cout<< "inserting chunk "<< current_partition - (char*)buf<<endl;

            if (p1.flags & END_BIT_MASK) {
                p2.ack_num = TERM_ACK;
                p2.flags = p2.flags | END_BIT_MASK;
                ucpSendTo(server_sockfd, &p2, sizeof(packet), &client_addr);
                terminated = TRUE;
                current_partition = (char *)buf + expected_seq * BUFFER_SIZE;
                memcpy (current_partition, p1.buff, p1.buff_len);
                continue;
            }
            // current_partition = current_partition + p1.buff_len;
            current_partition = (char *)buf + expected_seq * BUFFER_SIZE;
            memcpy (current_partition, p1.buff, p1.buff_len);
            expected_seq = (expected_seq + 1) % SYN_NUM_MAX;
            // cout<< "Sending ack#"<< p2.ack_num<<endl;
            ucpSendTo(server_sockfd, &p2, sizeof(packet), &client_addr);
        }
        bad_packet = 0;

    }
    return -1;
}

int rcsSend(int sockfd, void* buf, int len) {
    char * current_partition = (char *) buf;
    int i, buff_len, expected_ack = 0;
    int seq_window_start = 0;
    packet p1, p2;
    ucpSetSockRecvTimeout(sockfd, TIME_OUT);

    sockaddr_in server_addr;
    lookup_addr_map(sockfd, &server_addr);

    // int final_ack = -2300;

    // cout<< "Sending "<<endl;
    for(;;) {
        for (i = 0; i < WINDOW_SIZE; i ++) {
            initPacket(&p1);
            buff_len = BUFFER_SIZE;
            current_partition = (char *)buf + (seq_window_start + i) * BUFFER_SIZE;

            if ( (current_partition + BUFFER_SIZE - (char *)buf) >= len ) {
                // cout<< "Sending remaining"<<((char*)buf + len - current_partition)<<endl;
                buff_len = ((char*)buf + len - current_partition);
                p1.flags = p1.flags | END_BIT_MASK;
                // final_ack = (seq_window_start + i) % SYN_NUM_MAX;
                // cout<< "terminating"<<endl;
            }

            p1.seq_num = (seq_window_start + i) % SYN_NUM_MAX;
            p1.ack_num = ack_for_server;
            memcpy(p1.buff, current_partition, buff_len);
            p1.checksum = getCheckSum(p1.buff, buff_len);
            p1.buff_len = buff_len;
            p1.flags = p1.flags | ACK_BIT_MASK;
            p1.flags = p1.flags | SYN_BIT_MASK;

            // cout<< "Sending Seq#"<< p1.seq_num<<endl;
            ucpSendTo(sockfd, &p1, sizeof(packet), &server_addr);
            if (p1.flags & END_BIT_MASK) {
                break;
            }
        }
        for (i = 0; i < WINDOW_SIZE; i ++) {
            initPacket(&p2);
            ucpRecvFrom(sockfd, &p2, sizeof(packet), &server_addr);
            // cout<< "received ack#"<< p2.ack_num<<" expecting "<<expected_ack<<endl;
            if (p2.ack_num == CLOSE_ACK && p2.flags & END_BIT_MASK) {
                for (int j = 0; j < 12; j ++) {
                    ucpRecvFrom(sockfd, &p2, sizeof(packet), &server_addr);
                }
                return -1;
            }

            if (p2.ack_num == TERM_ACK && p2.flags & END_BIT_MASK) {
                for (int j = 0; j < 12; j ++) {
                    ucpRecvFrom(sockfd, &p2, sizeof(packet), &server_addr);
                }
                return 0;
            }
            if (p2.ack_num != expected_ack) {
                continue;
            }
            expected_ack = (expected_ack + 1) % SYN_NUM_MAX;
            seq_window_start = (seq_window_start + 1) % SYN_NUM_MAX;
        }
    }
    return -1;
}

int rcsClose(int sockfd) {
    sockaddr_in addr;
    packet p1;
    lookup_addr_map(sockfd, &addr);
    for (int i = 0; i < 10; i ++) {
        p1.flags = p1.flags | END_BIT_MASK;
        p1.ack_num = CLOSE_ACK;
        ucpSendTo(sockfd, &p1, sizeof(packet), &addr);
    }
    return 0;
}