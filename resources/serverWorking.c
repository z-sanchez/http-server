#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 18000
#define BUFFER_SIZE 1024

int main()
{
    int server_fd;
    struct sockaddr_in server_addr;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // config socket
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // bind socket to port
    if (bind(server_fd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // listen for connections
    if (listen(server_fd, 10) < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        char buffer[BUFFER_SIZE];

        // Allocate memory for client_fd
        int *client_fd = (int *)malloc(sizeof(int));
        if (client_fd == NULL)
        {
            perror("Memory allocation failed");
            continue;
        }

        // Accept incoming connection
        *client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (*client_fd < 0)
        {
            perror("Accept failed");
            free(client_fd);
            continue;
        }

        printf("Connection accepted from client IP %s:%d...\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Read request (optional, just to clear buffer)
        memset(buffer, 0, BUFFER_SIZE);
        read(*client_fd, buffer, BUFFER_SIZE);
        printf("Received request:\n%s\n", buffer);

        // Send response
        char *response = (char *)"HTTP/1.1 200 OK\r\n"
                                 "Content-Type: text/plain\r\n"
                                 "Content-Length: 15\r\n" // Specify content length
                                 "\r\n"
                                 "This is the end";

        send(*client_fd, response, strlen(response), 0);

        // Close client connection
        close(*client_fd);

        // Free allocated memory
        free(client_fd);
    }
}
