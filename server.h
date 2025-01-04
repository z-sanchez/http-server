#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h> // Core socket functions
#include <netinet/in.h> // Internet address structures and constants

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