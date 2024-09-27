#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
/*Reference : https://github.com/AkankshaSingal8/socket_programming/tree/main */

/* Function to make a request to the server */
void* make_request(void* client_id){
    int id = *(int*) client_id;
    free(client_id);
    int client_fd;
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1) {
        printf("Unsuccessful client socket creation\n");
        return NULL;
    }

    struct sockaddr_in server_address;
    /* IP address belongs to IPV4 family */
    server_address.sin_family = AF_INET;  
    server_address.sin_port = htons(1517);

    /*IP address of the server the client wants to connect to the local host */
    const char *server_ip_address = "127.0.0.1"; // local host
    if (inet_pton(AF_INET, server_ip_address, &server_address.sin_addr) <= 0) {
        printf("Invalid IP address for the server. Connection can't be established\n");
        close(client_fd);
        return -1;
    }

    /* Connecting to the server */
    if (connect(client_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        printf("Connection not established\n");
        close(client_fd);
        return -1;
    }


    /* Sending the request to the server to get top 2 CPU processes */
    char *client_request = "Give me the top CPU processes currently";
    send(client_fd, client_request, strlen(client_request), 0);

    /* Reading the response from the server */
    char server_response[2048];
    read(client_fd, server_response, sizeof(server_response));
    printf("Client no: %d, Server Response: \n", id + 1);
    printf("%s \n", server_response);

    /* Closing the socket */
    close(client_fd);
    return NULL;

}
int main(int argc, char* argv[]) {
    if (argc < 2){
        perror("More number of arguments expected. Add the number of client requests");
        return 0;
    }

    int num_of_client_requests = atoi(argv[1]);
    pthread_t client_reqs[num_of_client_requests];

    for (int i = 0; i < num_of_client_requests; i++){
        int* client_id = malloc(sizeof(int));
        *client_id = i;
        if (pthread_create(&client_reqs[i], NULL, make_request, client_id)){
            printf("Client thread creation not possible");
            free(client_id);
            continue;
        }

    }

    for (int i =0 ;i< num_of_client_requests; i++){
        pthread_join(client_reqs[i], NULL);
    }

}
