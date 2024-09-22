#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>


char* top_CPU_processes(){
    return "Hello ji";

}

void *serve_client(void* client_socket){
    int new_socket = *(int*) client_socket;  // got client socket in dereferenced state
    free(client_socket);

    char buffer[1024]; // reads the client requests
    read(new_socket, buffer, sizeof(buffer));

    char *cur_cpu_proc = top_CPU_processes();

    send(new_socket, cur_cpu_proc, strlen(cur_cpu_proc), 0);

    // free(cur_cpu_proc);
    close(new_socket);
    pthread_exit(NULL);

}
int main(){

    int server_fd; // file descriptor of server socket

    // create a TCP connection
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1){
        perror("TCP connection not established");
        exit(0);
    }

    //bind the server socket to a ip address and a port number
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;  // ip address belongs to IPV4 family
    server_address.sin_port = htons(1516);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // bind the socket
    if (bind(server_fd, (struct sockaddr*) &server_address, sizeof(server_address)) < 0){
        perror("Binding of the server socket is unsuccessful");
        exit(0);
    }

    //server is set to listen for the client requests
    if (listen(server_fd, 5) < 0){
        perror("Listening of the server socket is failed");
        exit(0);
    }

    printf("Reached till here \n");

    // server listens to allc lient, untill the code is dirupted
    int* client_socket;
    struct sockaddr_in client_address;
    socklen_t addrlen = sizeof(client_address);
    pthread_t thread_id;



    while (1){
        client_socket = malloc(sizeof(int));
        if ((*client_socket = accept(server_fd, (struct sockaddr *) &client_address, &addrlen)) < 0){
            perror("Server cant accept client request");
            free(client_socket);
            continue;
        }

        // prints the client address the request is accepted from
        printf("Accepted connection from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

        // create a new thread to handle the client
        if (pthread_create(&thread_id ,NULL ,serve_client, (void*)client_socket) != 0){
            perror("Multithreading failed. Server not able to serve the client");
            continue;  // look for other client requests
        }

        pthread_detach(thread_id);

    }

    close(server_fd);

}