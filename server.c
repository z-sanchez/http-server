// gcc server.c -o server

#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h> // Address manipulation functions

#define PORT "6969"
#define BUFFER_SIZE 1024

void log_error(char *message, int is_fatal)
{
    perror(message);

    if (is_fatal)
    {
        exit(EXIT_FAILURE);
    }
}

void print_ip(struct sockaddr_storage *client_addr)
{
    char client_ip[INET6_ADDRSTRLEN];
    int client_port = 0;

    if (client_addr->ss_family == AF_INET)
    {
        struct sockaddr_in *addr = (struct sockaddr_in *)client_addr;

        client_port = ntohs(addr->sin_port);
        inet_ntop(AF_INET, &(addr->sin_addr), client_ip, sizeof(client_ip));
    }
    else if (client_addr->ss_family == AF_INET6)
    {
        struct sockaddr_in6 *addr = (struct sockaddr_in6 *)client_addr;

        client_port = ntohs(addr->sin6_port);
        inet_ntop(AF_INET6, &addr->sin6_addr, client_ip, sizeof(client_ip));
    }
    else
    {
        printf("Unknown address family \n");
    }

    printf("\n Client connected: IP=%s, Port: %d\n\n\n", client_ip, client_port);
}

char *parse_request(char *request)
{
    static char method[16]; // Buffer to hold the HTTP method
    char path[256];         // Buffer to hold the path
    char protocol[16];      // Buffer to hold the HTTP protocol

    if (sscanf(request, "%15s %255s %15s", method, path, protocol) != 3)
    {
        // why use fprintf
        fprintf(stderr, "Malformed request: %s\n", request);
        return NULL;
    };

    return method;
}

struct Server server_constructor(int domain, u_long interface, char *port)
{
    struct Server server;
    server.domain = domain;
    server.interface = interface;
    server.port = port;

    memset(&server.hints, 0, sizeof server.hints);
    server.hints.ai_family = server.domain;
    server.hints.ai_socktype = server.interface;
    server.hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo("localhost", "6969", &server.hints, &server.res) != 0)
    {
        perror("getaddrinfo failed");
        exit(EXIT_FAILURE);
    }

    server.socket = socket(server.res->ai_family, server.res->ai_socktype, server.res->ai_protocol);

    if (server.socket < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set SO_REUSEADDR to allow immediate rebinding
    int opt = 1;
    if (setsockopt(server.socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        perror("setsockopt failed");
        close(server.socket);
        exit(EXIT_FAILURE);
    }

    // bind server fd and socket config
    if (bind(server.socket, server.res->ai_addr, server.res->ai_addrlen) < 0)
    {
        perror("bind failed");
        close(server.socket);
        exit(EXIT_FAILURE);
    }

    // listen for connections with max at 10
    if (listen(server.socket, 10) < 0)
    {
        perror("listen failed");
        close(server.socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %s\n", (char *)PORT);

    return server;
}

void handle_client(int client_fd, char *buffer)
{
    char *method = parse_request(buffer);
    if (!method)
    {
        const char *response = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nMalformed request";
        send(client_fd, response, strlen(response), 0);
        return;
    }

    char *response;

    printf("Received request:\n%s\n", buffer);

    printf("Method: %s\n", method);

    if (strcmp(method, "GET") == 0)
    {
        response = (char *)"HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/plain\r\n"
                           "\r\n"
                           "Successful Request";
    }
    else
    {
        response = (char *)"HTTP/1.1 404 Not Found\r\n"
                           "Content-Type: text/plain\r\n"
                           "\r\n"
                           "Resource not found";
    }

    send(client_fd, response, strlen(response), 0);
}

int main()
{
    // AF_INET = IPv4, SOCK_STREAM = TCP, protocol number (0 for standard sockets)
    struct Server server = server_constructor(AF_UNSPEC, SOCK_STREAM, (char *)PORT);

    while (1)
    {
        struct sockaddr_storage client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        // buffer for reading request
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);

        // client file descriptor
        // setting file descriptor after accepting connection
        int client_fd = accept(server.socket, (struct sockaddr *)&client_addr, &client_addr_len);

        if (client_fd < 0)
        {
            perror("Accept failed");
            continue;
        }

        print_ip(&client_addr);

        // uses file descriptor to read network communication
        ssize_t bytes_read = read(client_fd, buffer, BUFFER_SIZE);
        if (bytes_read < 0)
        {
            perror("Failed to read from client");
            close(client_fd);
            continue;
        }

        handle_client(client_fd, buffer);

        // Close client connection
        close(client_fd);
    }

    freeaddrinfo(server.res);

    return 0;
}