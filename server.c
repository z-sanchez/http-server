// gcc server.c -o server

#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h> // Address manipulation functions

#define PORT "6969"
#define CRLF "\r\n"
#define BUFFER_SIZE 1024

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

    printf("\nClient connected: IP=%s, Port: %d\n\n\n", client_ip, client_port);
}

int read_line(char *buffer, char **line)
{
    // find crlf
    char *crlf = strstr(buffer, CRLF);

    if (crlf == NULL)
    {
        return 0;
    }

    // pointer arithmetic to find length of line
    size_t line_length = crlf - buffer;

    // allocate space accordingly
    *line = (char *)malloc(line_length + 1);

    // copy line from buffer into our line pointer
    memcpy(*line, buffer, line_length);

    // add null terminate, parenthesis because we must get dereferenced value of line first
    (*line)[line_length] = '\0';

    // Shift remaining buffer contents
    size_t remaining_length = BUFFER_SIZE - (line_length + 2); // +2 for \r\n

    memmove(buffer, crlf + 2, remaining_length); // Shift data
    buffer[remaining_length] = '\0';             // Null-terminate the remaining buffer
    return 1;
}

void parse_request(char *buffer, struct RequestData *request_data)
{
    // must do static here for memory to persist
    static char method[16];   // Buffer to hold the HTTP method
    static char path[256];    // Buffer to hold the path
    static char protocol[16]; // Buffer to hold the HTTP protocol
    static char host[24];     // Buffer to hold the HTTP host
    static char content[BUFFER_SIZE];
    char *line = NULL;
    read_line(buffer, &line);

    if (sscanf(line, "%15s %255s %15s", method, path, protocol) != 3)
    {
        char error_msg[BUFFER_SIZE];
        snprintf(error_msg, sizeof(error_msg), "Malformed request line: '%s'", line);
        perror(error_msg);
        free(line);
        return;
    };

    request_data->method = method;
    request_data->path = path;
    request_data->protocol = protocol;

    read_line(buffer, &line);

    sscanf(line, "Host: %s", host);
    request_data->host = host;

    int status = 1;

    while (status == 1)
    {
        status = read_line(buffer, &line);

        if (strlen(line))
        {
            printf("%s\n", line);
        }
    }

    strcpy(content, buffer);

    request_data->content = content;

    free(line);
}

int get_file(char *requested_path, char **file_content)
{
    char *path = requested_path;
    char url[512];

    // if path is root '/' default to index.html
    if (path[0] == '/')
    {
        path++;
    }

    // if path is empty, default to index.html
    if (strlen(path) == 0)
    {
        strcpy(url, "./public/index.html");
    }
    else
    {
        snprintf(url, sizeof(url), "./public/%s", path);
    }

    // open file
    FILE *file_pointer;

    file_pointer = fopen(url, "r");

    // if invalid, return not found
    if (file_pointer == NULL)
    {
        char error_msg[BUFFER_SIZE];
        snprintf(error_msg, sizeof(error_msg), "Invalid Path: %s\n", requested_path);
        perror(error_msg);
        return 0;
    }

    fseek(file_pointer, 0, SEEK_END);
    long file_size = ftell(file_pointer);
    fseek(file_pointer, 0, SEEK_SET);

    *file_content = (char *)malloc(file_size);

    if (!file_content)
    {
        perror("Failed to allocate space for file content");
        fclose(file_pointer);
        return 0;
    }

    if (fread(*file_content, 1, file_size, file_pointer) != (size_t)file_size)
    {
        perror("Failed to read file");
        free(*file_content);
        fclose(file_pointer);
        return 0;
    }

    return 1;
}

void build_message(struct RequestData request_data, char **response_header, char **response_content)
{

    *response_header = (char *)malloc(BUFFER_SIZE);
    if (*response_header == NULL)
    {
        perror("Failed to allocate response header");
        return;
    }
    if (strcmp(request_data.method, "GET") == 0)
    {
        // printf("PATH HERE IN BUILD MESSAGE: %s\n", request_data.path);
        char *content;

        int status = get_file(request_data.path, &content);

        if (status == 0)
        {
            *response_header = (char *)"HTTP/1.1 404 Not Found\r\n"
                                       "Content-Type: text/plain\r\n"
                                       "\r\n"
                                       "Resource not found";
            return;
        }

        // write content length into header
        snprintf(*response_header, BUFFER_SIZE, "HTTP/1.1 200 OK\r\n"
                                                "Content-Type: text/html\r\n"
                                                "Content-Length: %ld\r\n"
                                                "\r\n",
                 strlen(content));

        *response_content = content;
    }
    else if (strcmp(request_data.method, "POST") == 0)
    {
        // Allocate memory for response content for POST
        *response_content = strdup(request_data.content);
        if (*response_content == NULL)
        {
            perror("Failed to allocate response content");
            free(*response_header);
            return;
        }

        snprintf(*response_header, BUFFER_SIZE, "HTTP/1.1 200 OK\r\n"
                                                "Content-Type: text/html\r\n"
                                                "Content-Length: %zu\r\n"
                                                "\r\n",
                 strlen(*response_content));
    }
    else
    {
        *response_header = (char *)"HTTP/1.1 404 Not Found\r\n"
                                   "Content-Type: text/plain\r\n"
                                   "\r\n"
                                   "Resource not found";
        *response_content = (char *)"Unsupported HTTP Method";
    }
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

    struct RequestData *request_data = (struct RequestData *)malloc(sizeof(struct RequestData));

    parse_request(buffer, request_data);

    if (!request_data->method)
    {
        const char *response = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nMalformed request";
        send(client_fd, response, strlen(response), 0);
        return;
    }

    char *response;
    char *response_content;

    printf("Method: %s\n", request_data->method);
    printf("Path: %s\n", request_data->path);
    printf("Protocol: %s\n", request_data->protocol);
    printf("Host: %s\n", request_data->host);

    build_message(*request_data, &response, &response_content);

    // send file headers with response size
    send(client_fd, response, strlen(response), 0);

    // send file content
    send(client_fd, response_content, strlen(response_content), 0);

    free(request_data);
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
        ssize_t bytes_read = recv(client_fd, buffer, BUFFER_SIZE, 0);
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