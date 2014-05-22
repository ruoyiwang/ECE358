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

map<int, map<int, string> > groups;

int initStudents () {
    string s, name, temp;
    int stu_num, group_num;
    while (!getline( cin, s ).eof() && !s.empty())
    {
        stringstream ss( s );
        if (ss >> temp && temp == "Group") {
            ss >> group_num;
        }
        else {
            getline ( ss, name );
            stu_num = atoi(temp.c_str());
            name.erase(name.find_last_not_of(" \n\r\t")+1);
            cout << name << endl;
            groups[group_num][stu_num] = name;
        }
    }
}

int main(int argc, char *argv[])
{
    struct ifaddrs *myaddrs, *ifa;
    void *in_addr;
    char buf[256];
    int new_fd, old_fd, portnum = 0;

    struct sockaddr_in my_addr, their_addr;
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

    if ((old_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(0);
    }

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(portnum);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    while (bind (old_fd, (const struct sockaddr *)(&my_addr), sizeof(struct sockaddr_in)) < 0) {
        my_addr.sin_port = htons(++portnum);
    }
    memset (&my_addr, 0, sizeof(struct sockaddr_in));

    // get the portnumber here

    if (getsockname(old_fd, (struct sockaddr *) (&my_addr), &addrlen) < 0) {
        perror ("getsockname");
        exit (0);
    }

    printf ("%hu\n", ntohs(my_addr.sin_port));
    
    initStudents();

    while ( 1 ){
        listen(old_fd, 10); 

        memset(buf, 0, 256);
        memset(&my_addr, 0, sizeof(struct sockaddr_in));

        new_fd = accept(old_fd, (struct sockaddr *)&their_addr, &addrlen);

        while ( 1 ) {
            memset(buf, 0, 256);
            memset(&my_addr, 0, sizeof(struct sockaddr_in));

            if (recvfrom(new_fd, buf, 256, 0, (struct sockaddr*) (&my_addr), &addrlen) < 0){
                perror("recvfrom");
                break;
            }
            // printf("Server received: %s\n", buf);

            stringstream ss(buf);
            string command;
            int group_id, stu_id;

            ss >> command;
            if (command == "STOP_SESSION") {
                // printf("Session stopped");
                close(new_fd);
                break;
            }
            else if ( command == "STOP" ) {
                // printf("Program stopped");
                close(new_fd);
                close(old_fd);
                return 0;
            }
            else if ( command == "GET" ) {
                ss >> group_id;
                ss >> stu_id;
              //  printf("get %d %d: %s", group_id, stu_id, groups[group_id][stu_id].c_str());
                send(new_fd, groups[group_id][stu_id].c_str(), strlen(groups[group_id][stu_id].c_str()) + 1, 0);
            }
        }
    }
    return 0;
}
