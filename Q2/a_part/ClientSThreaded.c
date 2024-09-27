#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

void* make_request(void* client_id){
    int id = *(int*) client_id;
    free(client_id);
    int client_fd;
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1) {
        printf("Unsuccessful client socket creation\n");
        return NULL;
    }

    // struct sockaddr_in client_address;
    // socklen_t address_length = sizeof(client_address);
    // if (getsockname(client_fd, (struct sockaddr *)&client_address, &address_length) == -1) {
    //     printf("Failed to get client socket name\n");
    // } else {
    //     printf("Client socket created on port: %d\n", ntohs(client_address.sin_port));
    // }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;  // ip address belongs to IPV4 family
    server_address.sin_port = htons(1516);

    // ip address of the server the client wants to connect to
    const char *server_ip_address = "127.0.0.1"; // local host
    if (inet_pton(AF_INET, server_ip_address, &server_address.sin_addr) <= 0) {
        printf("Invalid IP address for the server. Connection can't be established\n");
        close(client_fd);
        return NULL;
    }

    // connect to the server
    if (connect(client_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        printf("Connection not established\n");
        close(client_fd);
        return NULL;
    }

    // send the request
    char *client_request = "Give me the top CPU processes currently";
    send(client_fd, client_request, strlen(client_request), 0);

    // read the response from the server and print it out
    char server_response[2048];
    int bytes_read = read(client_fd, server_response, sizeof(server_response)-1);
    if (bytes_read > 0) {
        server_response[bytes_read] = '\0';  // Null-terminate the response
    }else {
        printf("Failed to read server response\n");
        close(client_fd);
        return NULL;
    }
    
    printf("Client no: %d, Server Response: \n", id + 1);
    printf("%s \n", server_response);

    // close the socket
    close(client_fd);
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc < 2){
        perror("More number of arguments expected. Add the number of client requests");
        return 0;
    }

    int num_of_client_requests = atoi(argv[1]);

    for (int i = 0; i < num_of_client_requests; i++){
        int* client_id = malloc(sizeof(int));
        *client_id = i;
        make_request(client_id);
    }

    return 0;
}
