#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>

struct Server
{
    int socket;
    struct sockaddr_in server_addr;
    int domain;
    u_long interface;
    int port;
};

struct Server server_constructor(int domain, u_long interface, int port);

#endif