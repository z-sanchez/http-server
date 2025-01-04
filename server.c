#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#define PORT 6969
#define BUFFER_SIZE 1024

char *parse_request(char *request)
{
    char method[16];   // Buffer to hold the HTTP method
    char path[256];    // Buffer to hold the path
    char protocol[16]; // Buffer to hold the HTTP protocol

    if (sscanf(request, "%15s %255s %15s", method, path, protocol) != 3)
    {
        perror("Malformed request line");
        exit(EXIT_FAILURE);
    };

    char *method_copy = (char *)malloc(strlen(method) + 1);
    if (method_copy == NULL)
    {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    strcpy(method_copy, method);

    return method_copy;
}

struct Server server_constructor(int domain, u_long interface, int port)
{
    struct Server server;
    server.domain = domain;
    server.interface = interface;
    server.port = port;

    server.server_addr.sin_family = server.domain;
    server.server_addr.sin_addr.s_addr = server.interface;
    server.server_addr.sin_port = htons(server.port);

    server.socket = socket(AF_INET, SOCK_STREAM, 0);

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
    if (bind(server.socket, (struct sockaddr *)&server.server_addr, sizeof(server.server_addr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // listen for connections with max at 10
    if (listen(server.socket, 10) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    return server;
}

void handle_client(int *client_fd, char *buffer)
{
    char *method = parse_request(buffer);
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
                           "404 You Fucking Up My Shit";
    }

    send(*client_fd, response, strlen(response), 0);
}

int main()
{
    // AF_INET = IPv4, SOCK_STREAM = TCP, protocol number (0 for standard sockets)
    struct Server server = server_constructor(AF_INET, INADDR_ANY, PORT);

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        // buffer for reading request
        char buffer[BUFFER_SIZE];

        // client file descriptor
        int *client_fd = (int *)malloc(sizeof(int));

        if (client_fd == NULL)
        {
            perror("Memory allocation failed");
            continue;
        }

        // setting file descriptor after accepting connection
        *client_fd = accept(server.socket, (struct sockaddr *)&client_addr, &client_addr_len);

        if (*client_fd < 0)
        {
            perror("Accept failed");
            free(client_fd);
            continue;
        }

        printf("Connection accepted from client IP %s:%d...\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        memset(buffer, 0, BUFFER_SIZE);

        // uses file descriptor to read network communication
        read(*client_fd, buffer, BUFFER_SIZE);

        handle_client(client_fd, buffer);

        // Close client connection
        close(*client_fd);

        // Free allocated memory
        free(client_fd);
    }

    return 0;
}