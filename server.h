#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h> // Core socket functions
#include <netinet/in.h> // Internet address structures and constants
#include <netdb.h>

struct Server
{
    int socket;
    struct sockaddr_in server_addr;
    struct addrinfo hints;
    struct addrinfo *res;
    int domain;
    u_long interface;
    char *port;
};

struct RequestData
{
    char *method;
    char *path;
    char *protocol;
    char *host;
};

struct Server server_constructor(int domain, u_long interface, char *port);

void log_error(char *message, int is_fatal);

#endif