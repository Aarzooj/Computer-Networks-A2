#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>

#define PORT 1517
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024

char* top_CPU_processes() {
    return "Hello ji";
}

int main() {
    int server_fd, new_socket, client_sockets[MAX_CLIENTS], max_sd, sd;
    struct sockaddr_in server_address, client_address;
    fd_set readfds;
    char buffer[BUFFER_SIZE];

    // Initialize client sockets array
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;
    }

    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to an address and port
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    // printf("Listening on port %d\n", PORT);
    printf("Reached till here \n");

    int addrlen = sizeof(client_address);

    while (1) {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add the server socket to the set
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        // Add client sockets to set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        // Wait for activity on one of the sockets
        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            perror("Select error");
        }

        // If there's activity on the server socket, it's a new connection
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr*)&client_address, (socklen_t*)&addrlen)) < 0) {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }

            printf("New connection from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

            // Add new socket to the client_sockets array
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    printf("Adding to list of sockets as %d\n", i);
                    break;
                }
            }
        }

        // Handle IO for each client socket
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];

            if (FD_ISSET(sd, &readfds)) {
                // Check if it was for closing, and read the incoming message
                int valread = read(sd, buffer, BUFFER_SIZE);
                if (valread == 0) {
                    // Client disconnected
                    getpeername(sd, (struct sockaddr*)&client_address, (socklen_t*)&addrlen);
                    printf("Host disconnected, IP %s, Port %d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

                    close(sd);
                    client_sockets[i] = 0;
                } else {
                    // Null-terminate the buffer and process the request
                    buffer[valread] = '\0';
                    printf("Received from client %d: %s\n", i, buffer);

                    // Simulate processing the request and get CPU process info
                    char* response = top_CPU_processes();

                    // Send the response back to the client
                    send(sd, response, strlen(response), 0);
                }
            }
        }
    }

    return 0;
}
